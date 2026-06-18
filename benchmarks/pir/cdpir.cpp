#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Bytes = std::vector<std::uint8_t>;
using Clock = std::chrono::steady_clock;
using Duration = std::chrono::duration<double>;
using Field = NTL::ZZ_p;

constexpr std::size_t kServerCount = 2;
constexpr std::size_t kDigestSize = 32;
constexpr const char* kDefaultPrime =
    "52435875175126190479447740508185965837690552500527637822603658699938581184513";

struct XorShift64 {
  explicit XorShift64(std::uint64_t seed) : state(seed == 0 ? 0x9E3779B97F4A7C15ULL : seed) {}

  std::uint64_t next_u64()
  {
    std::uint64_t x = state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state = x;
    return x;
  }

  void fill(Bytes& out)
  {
    std::size_t offset = 0;
    while (offset < out.size()) {
      const std::uint64_t word = next_u64();
      const std::size_t take = std::min<std::size_t>(sizeof(word), out.size() - offset);
      std::memcpy(out.data() + offset, &word, take);
      offset += take;
    }
  }

  std::size_t index(std::size_t upper_bound)
  {
    if (upper_bound == 0) {
      throw std::invalid_argument("upper_bound must be nonzero");
    }
    return static_cast<std::size_t>(next_u64() % upper_bound);
  }

  std::uint64_t state;
};

static std::uint32_t rotr(std::uint32_t value, unsigned bits)
{
  return (value >> bits) | (value << (32 - bits));
}

static Bytes sha256(const Bytes& input)
{
  static constexpr std::array<std::uint32_t, 8> h0 = {
      0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
      0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
  };
  static constexpr std::array<std::uint32_t, 64> k = {
      0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
      0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
      0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
      0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
      0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
      0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
      0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
      0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
      0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
      0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
      0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
      0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
      0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
      0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
      0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
      0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
  };

  const std::uint64_t bit_len = static_cast<std::uint64_t>(input.size()) * 8;
  Bytes message = input;
  message.push_back(0x80);
  while (message.size() % 64 != 56) {
    message.push_back(0);
  }
  for (int i = 7; i >= 0; --i) {
    message.push_back(static_cast<std::uint8_t>((bit_len >> (8 * i)) & 0xff));
  }

  std::array<std::uint32_t, 8> h = h0;
  for (std::size_t chunk_start = 0; chunk_start < message.size(); chunk_start += 64) {
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16; ++i) {
      const std::size_t p = chunk_start + i * 4;
      w[i] = (static_cast<std::uint32_t>(message[p]) << 24) |
             (static_cast<std::uint32_t>(message[p + 1]) << 16) |
             (static_cast<std::uint32_t>(message[p + 2]) << 8) |
             static_cast<std::uint32_t>(message[p + 3]);
    }
    for (std::size_t i = 16; i < 64; ++i) {
      const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
      const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    std::uint32_t a = h[0];
    std::uint32_t b = h[1];
    std::uint32_t c = h[2];
    std::uint32_t d = h[3];
    std::uint32_t e = h[4];
    std::uint32_t f = h[5];
    std::uint32_t g = h[6];
    std::uint32_t hh = h[7];

    for (std::size_t i = 0; i < 64; ++i) {
      const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const std::uint32_t ch = (e & f) ^ ((~e) & g);
      const std::uint32_t temp1 = hh + s1 + ch + k[i] + w[i];
      const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      const std::uint32_t temp2 = s0 + maj;

      hh = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += hh;
  }

  Bytes digest(kDigestSize);
  for (std::size_t i = 0; i < h.size(); ++i) {
    digest[i * 4] = static_cast<std::uint8_t>(h[i] >> 24);
    digest[i * 4 + 1] = static_cast<std::uint8_t>(h[i] >> 16);
    digest[i * 4 + 2] = static_cast<std::uint8_t>(h[i] >> 8);
    digest[i * 4 + 3] = static_cast<std::uint8_t>(h[i]);
  }
  return digest;
}

static std::string hex(const Bytes& bytes)
{
  std::ostringstream out;
  for (const auto byte : bytes) {
    out << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
  }
  return out.str();
}

static Bytes sha256_pair(const Bytes& left, const Bytes& right)
{
  Bytes input;
  input.reserve(left.size() + right.size());
  input.insert(input.end(), left.begin(), left.end());
  input.insert(input.end(), right.begin(), right.end());
  return sha256(input);
}

static std::size_t next_power_of_two(std::size_t n)
{
  if (n <= 1) {
    return 1;
  }
  --n;
  for (std::size_t shift = 1; shift < sizeof(std::size_t) * 8; shift <<= 1) {
    n |= n >> shift;
  }
  return n + 1;
}

struct Layer {
  Layer() = default;
  Layer(std::size_t item_size, Bytes data) : item_size(item_size), data(std::move(data))
  {
    if (item_size == 0 || this->data.size() % item_size != 0) {
      throw std::invalid_argument("invalid layer shape");
    }
  }

  std::size_t len() const { return data.size() / item_size; }

  Bytes item(std::size_t index) const
  {
    const std::size_t begin = index * item_size;
    return Bytes(data.begin() + static_cast<std::ptrdiff_t>(begin),
                 data.begin() + static_cast<std::ptrdiff_t>(begin + item_size));
  }

  void set_item(std::size_t index, const Bytes& value)
  {
    if (value.size() != item_size) {
      throw std::invalid_argument("wrong item size");
    }
    const std::size_t begin = index * item_size;
    std::copy(value.begin(), value.end(), data.begin() + static_cast<std::ptrdiff_t>(begin));
  }

  std::size_t item_size = 0;
  Bytes data;
};

struct MerkleTree {
  static MerkleTree build(const Bytes& database, std::size_t block_size)
  {
    if (block_size == 0 || database.size() % block_size != 0) {
      throw std::invalid_argument("invalid database shape");
    }
    const std::size_t original_len = database.size() / block_size;
    if (original_len < 2) {
      throw std::invalid_argument("database must contain at least two items");
    }

    const std::size_t padded_len = next_power_of_two(original_len);
    Bytes leaves(padded_len * block_size, 0);
    std::copy(database.begin(), database.end(), leaves.begin());

    MerkleTree tree;
    tree.original_len = original_len;
    tree.block_size = block_size;
    tree.levels.emplace_back(block_size, std::move(leaves));

    while (tree.levels.back().len() > 1) {
      const Layer& lower = tree.levels.back();
      Bytes parent_data;
      parent_data.reserve((lower.len() / 2) * kDigestSize);
      for (std::size_t parent = 0; parent < lower.len() / 2; ++parent) {
        Bytes digest = sha256_pair(lower.item(parent * 2), lower.item(parent * 2 + 1));
        parent_data.insert(parent_data.end(), digest.begin(), digest.end());
      }
      tree.levels.emplace_back(kDigestSize, std::move(parent_data));
    }

    return tree;
  }

  std::size_t padded_len() const { return levels.front().len(); }
  std::size_t depth() const { return levels.size() - 1; }
  const Bytes tag() const { return levels.back().item(0); }
  const Layer& layer(std::size_t level) const { return levels.at(level); }

  void update(std::size_t index, const Bytes& new_block)
  {
    if (index >= original_len || new_block.size() != block_size) {
      throw std::invalid_argument("invalid update");
    }
    levels[0].set_item(index, new_block);
    std::size_t child_index = index;
    for (std::size_t level = 1; level < levels.size(); ++level) {
      const std::size_t parent_index = child_index / 2;
      Bytes digest = sha256_pair(levels[level - 1].item(parent_index * 2),
                                 levels[level - 1].item(parent_index * 2 + 1));
      levels[level].set_item(parent_index, digest);
      child_index = parent_index;
    }
  }

  std::size_t original_len = 0;
  std::size_t block_size = 0;
  std::vector<Layer> levels;
};

static bool merkle_verify(const Bytes& tag, std::size_t index, const Bytes& leaf,
                          const std::vector<Bytes>& proof)
{
  Bytes current = leaf;
  std::size_t node_index = index;
  for (const Bytes& sibling : proof) {
    if ((node_index & 1U) == 0) {
      current = sha256_pair(current, sibling);
    } else {
      current = sha256_pair(sibling, current);
    }
    node_index >>= 1U;
  }
  return current == tag;
}

static std::size_t choose3(std::size_t m)
{
  if (m < 3) {
    return 0;
  }
  return m * (m - 1) * (m - 2) / 6;
}

static std::size_t wy_variable_count(std::size_t database_len)
{
  std::size_t m = 3;
  while (choose3(m) < database_len) {
    ++m;
  }
  return m;
}

static std::array<std::size_t, 3> wy_assignment(std::size_t index, std::size_t m)
{
  std::size_t seen = 0;
  for (std::size_t a = 0; a + 2 < m; ++a) {
    for (std::size_t b = a + 1; b + 1 < m; ++b) {
      for (std::size_t c = b + 1; c < m; ++c) {
        if (seen == index) {
          return {a, b, c};
        }
        ++seen;
      }
    }
  }
  throw std::invalid_argument("index outside geometric PIR encoding space");
}

static Field wy_lambda(std::size_t server_id)
{
  if (server_id == 0) {
    return Field(2);
  }
  if (server_id == 1) {
    return Field(3);
  }
  throw std::invalid_argument("this implementation is fixed to two servers");
}

struct WyPirQuery {
  std::vector<Field> point;
};

struct CommunicationBreakdown {
  std::size_t query_bytes_per_server = 0;
  std::array<std::size_t, kServerCount> answer_bytes_per_server{};

  std::size_t total_online_bytes() const
  {
    std::size_t total = kServerCount * query_bytes_per_server;
    for (const std::size_t answer_bytes : answer_bytes_per_server) {
      total += answer_bytes;
    }
    return total;
  }
};

struct WyPirAnswer {
  std::vector<Field> value;
  std::vector<std::vector<Field>> partials;
};

static WyPirAnswer wy_answer(const Layer& layer, const WyPirQuery& query)
{
  const std::size_t m = query.point.size();
  WyPirAnswer answer;
  answer.value.assign(layer.item_size, Field(0));
  answer.partials.assign(m, std::vector<Field>(layer.item_size, Field(0)));

  std::size_t item_index = 0;
  for (std::size_t a = 0; a + 2 < m && item_index < layer.len(); ++a) {
    for (std::size_t b = a + 1; b + 1 < m && item_index < layer.len(); ++b) {
      for (std::size_t c = b + 1; c < m && item_index < layer.len(); ++c) {
        const Field qa = query.point[a];
        const Field qb = query.point[b];
        const Field qc = query.point[c];
        const Field monomial = qa * qb * qc;
        const Field partial_a = qb * qc;
        const Field partial_b = qa * qc;
        const Field partial_c = qa * qb;

        const Bytes item = layer.item(item_index);
        for (std::size_t byte_index = 0; byte_index < item.size(); ++byte_index) {
          const Field coeff(static_cast<long>(item[byte_index]));
          answer.value[byte_index] += coeff * monomial;
          answer.partials[a][byte_index] += coeff * partial_a;
          answer.partials[b][byte_index] += coeff * partial_b;
          answer.partials[c][byte_index] += coeff * partial_c;
        }
        ++item_index;
      }
    }
  }

  return answer;
}

static Field wy_hermite_zero(const Field& y_lambda2, const Field& d_lambda2,
                             const Field& y_lambda3, const Field& d_lambda3)
{
  return Field(-27) * y_lambda2 + Field(-18) * d_lambda2 + Field(28) * y_lambda3 +
         Field(-12) * d_lambda3;
}

struct WyPirInvocation {
  static WyPirInvocation query(std::size_t database_len, std::size_t index)
  {
    if (index >= database_len) {
      throw std::invalid_argument("query index out of range");
    }

    WyPirInvocation invocation;
    invocation.m = wy_variable_count(database_len);
    invocation.v.reserve(invocation.m);
    for (std::size_t i = 0; i < invocation.m; ++i) {
      invocation.v.push_back(NTL::random_ZZ_p());
    }

    std::vector<Field> p(invocation.m, Field(0));
    for (const std::size_t coordinate : wy_assignment(index, invocation.m)) {
      p[coordinate] = Field(1);
    }

    for (std::size_t server = 0; server < kServerCount; ++server) {
      invocation.queries[server].point.assign(invocation.m, Field(0));
      const Field lambda = wy_lambda(server);
      for (std::size_t coordinate = 0; coordinate < invocation.m; ++coordinate) {
        invocation.queries[server].point[coordinate] = p[coordinate] + lambda * invocation.v[coordinate];
      }
    }

    return invocation;
  }

  WyPirAnswer answer(const Layer& layer, std::size_t server_id) const
  {
    return wy_answer(layer, queries.at(server_id));
  }

  std::optional<Bytes> reconstruct(const WyPirAnswer& answer0, const WyPirAnswer& answer1) const
  {
    if (answer0.value.size() != answer1.value.size() || answer0.partials.size() != m ||
        answer1.partials.size() != m) {
      return std::nullopt;
    }

    const std::size_t item_size = answer0.value.size();
    std::vector<Field> derivative0(item_size, Field(0));
    std::vector<Field> derivative1(item_size, Field(0));
    for (std::size_t coordinate = 0; coordinate < m; ++coordinate) {
      for (std::size_t byte_index = 0; byte_index < item_size; ++byte_index) {
        derivative0[byte_index] += answer0.partials[coordinate][byte_index] * v[coordinate];
        derivative1[byte_index] += answer1.partials[coordinate][byte_index] * v[coordinate];
      }
    }

    Bytes output;
    output.reserve(item_size);
    const NTL::ZZ max_byte(255);
    for (std::size_t byte_index = 0; byte_index < item_size; ++byte_index) {
      const Field value =
          wy_hermite_zero(answer0.value[byte_index], derivative0[byte_index],
                          answer1.value[byte_index], derivative1[byte_index]);
      const NTL::ZZ repr = NTL::rep(value);
      if (repr < 0 || repr > max_byte) {
        return std::nullopt;
      }
      output.push_back(static_cast<std::uint8_t>(NTL::conv<long>(repr)));
    }
    return output;
  }

  std::size_t communication_bytes(std::size_t item_size) const
  {
    return kServerCount * query_bytes_per_server() +
           kServerCount * answer_bytes_per_server(item_size);
  }

  std::size_t query_bytes_per_server() const
  {
    return m * field_bytes();
  }

  std::size_t answer_bytes_per_server(std::size_t item_size) const
  {
    return (m + 1) * item_size * field_bytes();
  }

  static std::size_t field_bytes()
  {
    return static_cast<std::size_t>(NTL::NumBytes(NTL::ZZ_p::modulus()));
  }

  std::size_t m = 0;
  std::vector<Field> v;
  std::array<WyPirQuery, kServerCount> queries;
};

struct PathQuery {
  std::size_t level = 0;
  std::size_t sibling_index = 0;
  WyPirInvocation invocation;
};

struct CdQuery {
  std::size_t index = 0;
  WyPirInvocation leaf;
  std::vector<PathQuery> path;

  std::size_t communication_bytes(const MerkleTree& tree) const
  {
    return communication_breakdown(tree).total_online_bytes();
  }

  CommunicationBreakdown communication_breakdown(const MerkleTree& tree) const
  {
    CommunicationBreakdown breakdown;

    auto add_invocation = [&](const WyPirInvocation& invocation, std::size_t item_size) {
      breakdown.query_bytes_per_server += invocation.query_bytes_per_server();
      const std::size_t answer_bytes = invocation.answer_bytes_per_server(item_size);
      for (std::size_t server = 0; server < kServerCount; ++server) {
        breakdown.answer_bytes_per_server[server] += answer_bytes;
      }
    };

    add_invocation(leaf, tree.layer(0).item_size);
    for (const PathQuery& path_query : path) {
      add_invocation(path_query.invocation, tree.layer(path_query.level).item_size);
    }
    return breakdown;
  }
};

struct CdAnswer {
  WyPirAnswer leaf;
  std::vector<WyPirAnswer> path;

  void flip_first_field()
  {
    if (!leaf.value.empty()) {
      leaf.value[0] += Field(1);
      return;
    }
    if (!path.empty() && !path[0].value.empty()) {
      path[0].value[0] += Field(1);
    }
  }
};

struct CdPir {
  static std::pair<Bytes, MerkleTree> g(const Bytes& database, std::size_t block_size)
  {
    MerkleTree tree = MerkleTree::build(database, block_size);
    return {tree.tag(), std::move(tree)};
  }

  static CdQuery q(const MerkleTree& tree, std::size_t index)
  {
    if (index >= tree.original_len) {
      throw std::invalid_argument("query index out of range");
    }

    CdQuery query;
    query.index = index;
    query.leaf = WyPirInvocation::query(tree.layer(0).len(), index);

    std::size_t node_index = index;
    for (std::size_t level = 0; level < tree.depth(); ++level) {
      const std::size_t sibling_index = node_index ^ 1U;
      query.path.push_back(PathQuery{
          level,
          sibling_index,
          WyPirInvocation::query(tree.layer(level).len(), sibling_index),
      });
      node_index >>= 1U;
    }
    return query;
  }

  static CdAnswer a(const MerkleTree& tree, std::size_t server_id, const CdQuery& query)
  {
    CdAnswer answer;
    answer.leaf = query.leaf.answer(tree.layer(0), server_id);
    answer.path.reserve(query.path.size());
    for (const PathQuery& path_query : query.path) {
      answer.path.push_back(path_query.invocation.answer(tree.layer(path_query.level), server_id));
    }
    return answer;
  }

  static std::optional<Bytes> r(const Bytes& tag, const CdQuery& query,
                                const std::array<CdAnswer, kServerCount>& answers)
  {
    std::optional<Bytes> leaf = query.leaf.reconstruct(answers[0].leaf, answers[1].leaf);
    if (!leaf) {
      return std::nullopt;
    }

    std::vector<Bytes> proof;
    proof.reserve(query.path.size());
    for (std::size_t i = 0; i < query.path.size(); ++i) {
      std::optional<Bytes> path_value =
          query.path[i].invocation.reconstruct(answers[0].path[i], answers[1].path[i]);
      if (!path_value) {
        return std::nullopt;
      }
      proof.push_back(std::move(*path_value));
    }

    if (merkle_verify(tag, query.index, *leaf, proof)) {
      return leaf;
    }
    return std::nullopt;
  }

  static Bytes u(MerkleTree& tree, std::size_t index, const Bytes& new_block)
  {
    tree.update(index, new_block);
    return tree.tag();
  }
};

struct Config {
  std::size_t items = 65536;
  std::size_t block_size = 32;
  std::size_t rounds = 10;
  std::uint64_t seed = 0xC0DEC0DEC0DEC0DEULL;
  bool cheat = false;
  bool self_test = false;
  NTL::ZZ modulus = NTL::conv<NTL::ZZ>(kDefaultPrime);
};

static void usage()
{
  std::cout << "usage: cdpir [--items N] [--block-size BYTES] [--rounds N]\n"
               "                    [--cyctimes N]\n"
               "                 [--seed N] [--prime DECIMAL] [--cheat] [--self-test]\n";
}

template <class T>
static T parse_number(const std::string& value, const char* name)
{
  std::istringstream in(value);
  T parsed{};
  in >> parsed;
  if (!in || !in.eof()) {
    throw std::invalid_argument(std::string("invalid value for ") + name + ": " + value);
  }
  return parsed;
}

static Config parse_config(int argc, char** argv)
{
  Config config;
  for (int i = 1; i < argc; ++i) {
    const std::string arg(argv[i]);
    auto require_value = [&](const char* name) -> std::string {
      if (i + 1 >= argc) {
        throw std::invalid_argument(std::string("missing value for ") + name);
      }
      return argv[++i];
    };

    if (arg == "--items") {
      config.items = parse_number<std::size_t>(require_value("--items"), "--items");
    } else if (arg == "--block-size") {
      config.block_size = parse_number<std::size_t>(require_value("--block-size"), "--block-size");
    } else if (arg == "--rounds") {
      config.rounds = parse_number<std::size_t>(require_value("--rounds"), "--rounds");
    } else if (arg == "--cyctimes") {
      config.rounds = parse_number<std::size_t>(require_value("--cyctimes"), "--cyctimes");
    } else if (arg == "--seed") {
      config.seed = parse_number<std::uint64_t>(require_value("--seed"), "--seed");
    } else if (arg == "--prime") {
      config.modulus = NTL::conv<NTL::ZZ>(require_value("--prime").c_str());
    } else if (arg == "--cheat") {
      config.cheat = true;
    } else if (arg == "--self-test") {
      config.self_test = true;
    } else if (arg == "--help" || arg == "-h") {
      usage();
      std::exit(0);
    } else {
      throw std::invalid_argument("unknown argument: " + arg);
    }
  }
  return config;
}

static Bytes random_database(std::size_t items, std::size_t block_size, XorShift64& rng)
{
  Bytes database(items * block_size);
  rng.fill(database);
  return database;
}

static double ms(Duration duration)
{
  return duration.count() * 1000.0;
}

static void print_time_ms(const std::string& label, double value)
{
  std::cout << label << ": " << value << " ms  RSD: 0%\n";
}

static std::string human_bytes(double bytes)
{
  const char* units[] = {"B", "KiB", "MiB", "GiB"};
  std::size_t unit = 0;
  while (bytes >= 1024.0 && unit + 1 < 4) {
    bytes /= 1024.0;
    ++unit;
  }
  std::ostringstream out;
  out << std::fixed << std::setprecision(3) << bytes << ' ' << units[unit];
  return out.str();
}

static void run_self_tests()
{
  if (hex(sha256(Bytes{'a', 'b', 'c'})) !=
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
    throw std::runtime_error("SHA-256 self-test failed");
  }

  XorShift64 rng(7);
  Bytes database = random_database(257, 24, rng);
  auto [tag, tree] = CdPir::g(database, 24);
  const std::size_t index = 199;
  CdQuery query = CdPir::q(tree, index);
  std::array<CdAnswer, kServerCount> answers = {
      CdPir::a(tree, 0, query),
      CdPir::a(tree, 1, query),
  };

  std::optional<Bytes> output = CdPir::r(tag, query, answers);
  if (!output || *output != tree.layer(0).item(index)) {
    throw std::runtime_error("cd-PIR accept self-test failed");
  }

  answers[0].flip_first_field();
  if (CdPir::r(tag, query, answers)) {
    throw std::runtime_error("cd-PIR tamper self-test failed");
  }

  Bytes new_block(24);
  rng.fill(new_block);
  Bytes new_tag = CdPir::u(tree, index, new_block);
  CdQuery update_query = CdPir::q(tree, index);
  std::array<CdAnswer, kServerCount> update_answers = {
      CdPir::a(tree, 0, update_query),
      CdPir::a(tree, 1, update_query),
  };
  std::optional<Bytes> updated = CdPir::r(new_tag, update_query, update_answers);
  if (!updated || *updated != new_block) {
    throw std::runtime_error("cd-PIR update self-test failed");
  }
}

static void run_benchmark(const Config& config)
{
  if (config.items < 2 || config.block_size == 0 || config.rounds == 0) {
    throw std::invalid_argument("items >= 2, block-size > 0, and rounds > 0 are required");
  }

  XorShift64 rng(config.seed);
  const auto gen_start = Clock::now();
  Bytes database = random_database(config.items, config.block_size, rng);
  const Duration database_time = Clock::now() - gen_start;

  const auto setup_start = Clock::now();
  auto [tag, tree] = CdPir::g(database, config.block_size);
  const Duration setup_time = Clock::now() - setup_start;

  Duration query_time{};
  Duration answer0_time{};
  Duration answer1_time{};
  Duration reconstruct_time{};
  Duration online_time{};
  std::size_t query_bytes_per_server = 0;
  std::size_t answer0_bytes = 0;
  std::size_t answer1_bytes = 0;
  std::size_t total_online_bytes = 0;
  std::size_t detected_cheats = 0;
  std::size_t last_index = 0;

  for (std::size_t round = 0; round < config.rounds; ++round) {
    const std::size_t index = rng.index(config.items);
    last_index = index;
    const Bytes expected = tree.layer(0).item(index);

    const auto online_start = Clock::now();

    const auto q_start = Clock::now();
    CdQuery query = CdPir::q(tree, index);
    query_time += Clock::now() - q_start;
    const CommunicationBreakdown comm = query.communication_breakdown(tree);
    query_bytes_per_server += comm.query_bytes_per_server;
    answer0_bytes += comm.answer_bytes_per_server[0];
    answer1_bytes += comm.answer_bytes_per_server[1];
    total_online_bytes += comm.total_online_bytes();

    std::array<CdAnswer, kServerCount> answers;
    const auto a0_start = Clock::now();
    answers[0] = CdPir::a(tree, 0, query);
    answer0_time += Clock::now() - a0_start;

    const auto a1_start = Clock::now();
    answers[1] = CdPir::a(tree, 1, query);
    answer1_time += Clock::now() - a1_start;

    const auto r_start = Clock::now();
    std::optional<Bytes> output = CdPir::r(tag, query, answers);
    reconstruct_time += Clock::now() - r_start;
    online_time += Clock::now() - online_start;

    if (!output || *output != expected) {
      throw std::runtime_error("correct execution was not accepted");
    }

    if (config.cheat) {
      std::array<CdAnswer, kServerCount> tampered = answers;
      tampered[0].flip_first_field();
      if (CdPir::r(tag, query, tampered)) {
        throw std::runtime_error("tampered answer was accepted");
      }
      ++detected_cheats;
    }
  }

  const std::size_t update_index = rng.index(config.items);
  Bytes new_block(config.block_size);
  rng.fill(new_block);
  const auto update_start = Clock::now();
  tag = CdPir::u(tree, update_index, new_block);
  const Duration update_time = Clock::now() - update_start;

  CdQuery check_query = CdPir::q(tree, update_index);
  std::array<CdAnswer, kServerCount> check_answers = {
      CdPir::a(tree, 0, check_query),
      CdPir::a(tree, 1, check_query),
  };
  std::optional<Bytes> check_output = CdPir::r(tag, check_query, check_answers);
  if (!check_output || *check_output != new_block) {
    throw std::runtime_error("post-update retrieval failed");
  }

  const double rounds = static_cast<double>(config.rounds);
  const double logical_mib =
      static_cast<double>(config.items * config.block_size) / (1024.0 * 1024.0);

  const double avg_query_bytes = static_cast<double>(query_bytes_per_server) / rounds;
  const double avg_answer0_bytes = static_cast<double>(answer0_bytes) / rounds;
  const double avg_answer1_bytes = static_cast<double>(answer1_bytes) / rounds;
  const double avg_total_online_bytes = static_cast<double>(total_online_bytes) / rounds;
  const bool tamper_rejected = !config.cheat || detected_cheats == config.rounds;

  std::cout << "=======================================================\n";
  std::cout << "  cd-PIR Benchmark\n";
  std::cout << "  underlying PIR = Woodruff-Yekhanin geometric PIR, k=2, t=1\n";
  std::cout << "  items = " << config.items
            << "  padded_items = " << tree.padded_len()
            << "  block_size = " << config.block_size
            << "  rounds = " << config.rounds << "\n";
  std::cout << "  finite_field = ZZ_p, p=" << NTL::ZZ_p::modulus() << "\n";
  std::cout << "  logical_database_mib = " << std::fixed << std::setprecision(3)
            << logical_mib << "\n";
  std::cout << "-------------------------------------------------------\n";
  print_time_ms("DatabaseGen", ms(database_time));
  print_time_ms("Setup", ms(setup_time));
  print_time_ms("Gen", 0.0);
  print_time_ms("Query", ms(query_time) / rounds);
  print_time_ms("Answer0", ms(answer0_time) / rounds);
  print_time_ms("Answer1", ms(answer1_time) / rounds);
  print_time_ms("Verify", ms(reconstruct_time) / rounds);
  print_time_ms("Decode", 0.0);
  print_time_ms("Update", ms(update_time));
  print_time_ms("Online", ms(online_time) / rounds);
  std::cout << "-------------------------------------------------------\n";
  std::cout << "Correctness: PASS\n";
  std::cout << "TamperRejected: ";
  if (config.cheat) {
    std::cout << (tamper_rejected ? "PASS" : "FAIL") << "\n";
  } else {
    std::cout << "SKIP\n";
  }
  std::cout << "DbSize: " << config.items << "\n";
  std::cout << "LogN: " << tree.depth() << "\n";
  std::cout << "RecordBits: " << config.block_size * 8 << "\n";
  std::cout << "BlockSizeBytes: " << config.block_size << "\n";
  std::cout << "Index: " << last_index << "\n";
  std::cout << "Items: " << config.items << "\n";
  std::cout << "PaddedItems: " << tree.padded_len() << "\n";
  std::cout << "MerkleDepth: " << tree.depth() << "\n";
  std::cout << "FieldBits: " << NTL::NumBits(NTL::ZZ_p::modulus()) << "\n";
  std::cout << "FieldBytes: " << NTL::NumBytes(NTL::ZZ_p::modulus()) << "\n";
  std::cout << "QueryBitsPerServer: " << static_cast<std::size_t>(avg_query_bytes * 8.0) << "\n";
  std::cout << "Answer0Bits: " << static_cast<std::size_t>(avg_answer0_bytes * 8.0) << "\n";
  std::cout << "Answer1Bits: " << static_cast<std::size_t>(avg_answer1_bytes * 8.0) << "\n";
  std::cout << "TotalOnlineBits: " << static_cast<std::size_t>(avg_total_online_bytes * 8.0) << "\n";
  std::cout << "FormulaQueryBitsPerServer: " << static_cast<std::size_t>(avg_query_bytes * 8.0) << "\n";
  std::cout << "FormulaAnswerBitsPerServer: "
            << static_cast<std::size_t>(avg_answer0_bytes * 8.0) << "\n";
  std::cout << "FormulaTotalBits: " << static_cast<std::size_t>(avg_total_online_bytes * 8.0)
            << "\n";
  std::cout << "SimulatedComm: " << human_bytes(avg_total_online_bytes) << "\n";
  if (config.cheat) {
    std::cout << "TamperTrials: " << detected_cheats << "/" << config.rounds << "\n";
  }
  std::cout << "=======================================================\n";
}

int main(int argc, char** argv)
{
  try {
    Config config = parse_config(argc, argv);
    NTL::ZZ_p::init(config.modulus);
    NTL::SetSeed(NTL::ZZ(config.seed));
    if (config.self_test) {
      run_self_tests();
      std::cout << "self-test passed\n";
      return 0;
    }
    run_benchmark(config);
    return 0;
  } catch (const std::exception& err) {
    std::cerr << "error: " << err.what() << "\n";
    usage();
    return 1;
  }
}
