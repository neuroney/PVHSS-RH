#include <sys/stat.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

struct MaybeDouble
{
    bool has;
    double value;
};

struct IndexKey
{
    string backend;
    string scheme;
    int degree;
    string phase;

    bool operator<(const IndexKey &other) const
    {
        return tie(backend, scheme, degree, phase) <
               tie(other.backend, other.scheme, other.degree, other.phase);
    }
};

static string trim(const string &value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos)
    {
        return "";
    }
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

static string lower_copy(string value)
{
    transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(tolower(c));
    });
    return value;
}

static string join_path(const string &left, const string &right)
{
    if (left.empty())
    {
        return right;
    }
    if (left[left.size() - 1] == '/')
    {
        return left + right;
    }
    return left + "/" + right;
}

static void make_dirs(const string &path)
{
    if (path.empty())
    {
        return;
    }
    size_t pos = (path[0] == '/') ? 1 : 0;
    while (pos <= path.size())
    {
        pos = path.find('/', pos);
        const string dir = path.substr(0, pos);
        if (!dir.empty() && mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST)
        {
            throw runtime_error("mkdir failed for " + dir + ": " + strerror(errno));
        }
        if (pos == string::npos)
        {
            break;
        }
        ++pos;
    }
}

static string csv_escape(const string &value)
{
    bool needs_quotes = false;
    string escaped;
    for (size_t i = 0; i < value.size(); ++i)
    {
        const char c = value[i];
        if (c == '"' || c == ',' || c == '\n' || c == '\r')
        {
            needs_quotes = true;
        }
        if (c == '"')
        {
            escaped += "\"\"";
        }
        else
        {
            escaped += c;
        }
    }
    return needs_quotes ? "\"" + escaped + "\"" : escaped;
}

static vector<string> parse_csv_line(const string &line)
{
    vector<string> fields;
    string field;
    bool quoted = false;
    for (size_t i = 0; i < line.size(); ++i)
    {
        const char c = line[i];
        if (quoted)
        {
            if (c == '"' && i + 1 < line.size() && line[i + 1] == '"')
            {
                field += '"';
                ++i;
            }
            else if (c == '"')
            {
                quoted = false;
            }
            else
            {
                field += c;
            }
        }
        else if (c == '"')
        {
            quoted = true;
        }
        else if (c == ',')
        {
            fields.push_back(field);
            field.clear();
        }
        else
        {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

static int column_index(const map<string, int> &columns, const string &name)
{
    map<string, int>::const_iterator it = columns.find(name);
    if (it == columns.end())
    {
        throw runtime_error("missing CSV column: " + name);
    }
    return it->second;
}

static map<IndexKey, vector<double> > read_protocol_components(const string &path)
{
    ifstream input(path.c_str());
    if (!input)
    {
        throw runtime_error("missing protocol component CSV: " + path);
    }

    string line;
    if (!getline(input, line))
    {
        throw runtime_error("empty protocol component CSV: " + path);
    }

    const vector<string> headers = parse_csv_line(line);
    map<string, int> columns;
    for (size_t i = 0; i < headers.size(); ++i)
    {
        columns[trim(headers[i])] = static_cast<int>(i);
    }

    const int backend_col = column_index(columns, "backend");
    const int scheme_col = column_index(columns, "scheme");
    const int degree_col = column_index(columns, "degree");
    const int phase_col = column_index(columns, "phase");
    const int mean_col = column_index(columns, "mean_ms");

    map<IndexKey, vector<double> > index;
    while (getline(input, line))
    {
        if (trim(line).empty())
        {
            continue;
        }
        const vector<string> row = parse_csv_line(line);
        const int needed = max(max(backend_col, scheme_col), max(max(degree_col, phase_col), mean_col));
        if (static_cast<int>(row.size()) <= needed)
        {
            continue;
        }

        IndexKey key;
        key.backend = lower_copy(trim(row[backend_col]));
        key.scheme = lower_copy(trim(row[scheme_col]));
        key.degree = atoi(trim(row[degree_col]).c_str());
        key.phase = trim(row[phase_col]);
        const double mean_ms = strtod(trim(row[mean_col]).c_str(), NULL);
        if (key.backend.empty() || key.scheme.empty() || key.degree <= 0 || key.phase.empty())
        {
            continue;
        }
        index[key].push_back(mean_ms);
    }
    return index;
}

static MaybeDouble mean(const vector<double> &values)
{
    if (values.empty())
    {
        return MaybeDouble{false, 0.0};
    }
    double total = 0.0;
    for (size_t i = 0; i < values.size(); ++i)
    {
        total += values[i];
    }
    return MaybeDouble{true, total / values.size()};
}

static MaybeDouble mean_phase(const map<IndexKey, vector<double> > &index,
                              const string &backend, const string &scheme,
                              int degree, const string &phase)
{
    IndexKey key{backend, scheme, degree, phase};
    map<IndexKey, vector<double> >::const_iterator it = index.find(key);
    if (it == index.end())
    {
        return MaybeDouble{false, 0.0};
    }
    return mean(it->second);
}

static MaybeDouble max_phase(const map<IndexKey, vector<double> > &index,
                             const string &backend, const string &scheme,
                             int degree, const vector<string> &phases)
{
    vector<double> values;
    for (size_t i = 0; i < phases.size(); ++i)
    {
        IndexKey key{backend, scheme, degree, phases[i]};
        map<IndexKey, vector<double> >::const_iterator it = index.find(key);
        if (it != index.end())
        {
            values.insert(values.end(), it->second.begin(), it->second.end());
        }
    }
    if (values.empty())
    {
        return MaybeDouble{false, 0.0};
    }
    return MaybeDouble{true, *max_element(values.begin(), values.end())};
}

static MaybeDouble sum_present(const MaybeDouble &a, const MaybeDouble &b)
{
    if (!a.has && !b.has)
    {
        return MaybeDouble{false, 0.0};
    }
    return MaybeDouble{true, (a.has ? a.value : 0.0) + (b.has ? b.value : 0.0)};
}

static MaybeDouble setup_total(const map<IndexKey, vector<double> > &index,
                               const string &backend, const string &scheme, int degree)
{
    const MaybeDouble direct = mean_phase(index, backend, scheme, degree, "Setup");
    if (direct.has)
    {
        return direct;
    }
    return sum_present(
        mean_phase(index, backend, scheme, degree, "SetupBase"),
        mean_phase(index, backend, scheme, degree, "SetupIncremental"));
}

static MaybeDouble gen_total(const map<IndexKey, vector<double> > &index,
                             const string &backend, const string &scheme, int degree)
{
    const MaybeDouble direct = mean_phase(index, backend, scheme, degree, "Gen");
    if (direct.has)
    {
        return direct;
    }
    return mean_phase(index, backend, scheme, degree, "GenIncremental");
}

static MaybeDouble eval_total(const map<IndexKey, vector<double> > &index,
                              const string &backend, const string &scheme, int degree)
{
    const MaybeDouble direct = max_phase(index, backend, scheme, degree,
                                         vector<string>{"Eval", "Eval0", "Eval1"});
    if (direct.has)
    {
        return direct;
    }

    vector<double> per_server_totals;
    for (int server = 0; server < 2; ++server)
    {
        const string suffix = server == 0 ? "0" : "1";
        const MaybeDouble total = sum_present(
            mean_phase(index, backend, scheme, degree, "EvalBase" + suffix),
            mean_phase(index, backend, scheme, degree, "EvalIncremental" + suffix));
        if (total.has)
        {
            per_server_totals.push_back(total.value);
        }
    }
    if (!per_server_totals.empty())
    {
        return MaybeDouble{true, *max_element(per_server_totals.begin(), per_server_totals.end())};
    }

    return sum_present(
        mean_phase(index, backend, scheme, degree, "EvalBase"),
        mean_phase(index, backend, scheme, degree, "EvalIncremental"));
}

static MaybeDouble direct_increment(const map<IndexKey, vector<double> > &index,
                                    const string &backend, const string &scheme,
                                    int degree, const string &stage)
{
    if (stage == "setup")
    {
        return mean_phase(index, backend, scheme, degree, "SetupIncremental");
    }
    if (stage == "gen")
    {
        return mean_phase(index, backend, scheme, degree, "GenIncremental");
    }
    if (stage == "eval")
    {
        return max_phase(index, backend, scheme, degree,
                         vector<string>{"EvalIncremental", "EvalIncremental0", "EvalIncremental1"});
    }
    return MaybeDouble{false, 0.0};
}

static MaybeDouble stage_total(const map<IndexKey, vector<double> > &index,
                               const string &backend, const string &scheme,
                               int degree, const string &stage)
{
    if (stage == "setup")
        return setup_total(index, backend, scheme, degree);
    if (stage == "gen")
        return gen_total(index, backend, scheme, degree);
    if (stage == "input")
        return mean_phase(index, backend, scheme, degree, "Input");
    if (stage == "eval")
        return eval_total(index, backend, scheme, degree);
    if (stage == "verify")
        return mean_phase(index, backend, scheme, degree, "Verify");
    if (stage == "decode")
        return mean_phase(index, backend, scheme, degree, "Decode");
    throw runtime_error("unknown stage: " + stage);
}

static MaybeDouble incremental_value(const map<IndexKey, vector<double> > &index,
                                     const string &backend, const string &scheme,
                                     const string &parent, int degree,
                                     const string &stage)
{
    const MaybeDouble direct = direct_increment(index, backend, scheme, degree, stage);
    if (direct.has)
    {
        return direct;
    }

    const MaybeDouble scheme_value = stage_total(index, backend, scheme, degree, stage);
    if (!scheme_value.has)
    {
        return MaybeDouble{false, 0.0};
    }

    const MaybeDouble parent_value = stage_total(index, backend, parent, degree, stage);
    if (stage == "input" && parent_value.has)
    {
        return MaybeDouble{true, 0.0};
    }
    if (!parent_value.has)
    {
        return scheme_value;
    }
    return MaybeDouble{true, scheme_value.value - parent_value.value};
}

static string format_ms(const MaybeDouble &value)
{
    if (!value.has)
    {
        return "";
    }
    ostringstream out;
    out << fixed << setprecision(6) << value.value;
    return out.str();
}

static int scheme_order(const string &scheme)
{
    if (scheme == "ot")
        return 0;
    if (scheme == "dc")
        return 1;
    return 99;
}

static vector<tuple<string, string, int> > protocol_keys(const map<IndexKey, vector<double> > &index)
{
    set<tuple<string, string, int> > keys;
    for (map<IndexKey, vector<double> >::const_iterator it = index.begin(); it != index.end(); ++it)
    {
        if (it->first.scheme == "ot" || it->first.scheme == "dc")
        {
            keys.insert(make_tuple(it->first.backend, it->first.scheme, it->first.degree));
        }
    }

    vector<tuple<string, string, int> > ordered(keys.begin(), keys.end());
    sort(ordered.begin(), ordered.end(), [](const tuple<string, string, int> &a,
                                            const tuple<string, string, int> &b) {
        if (get<0>(a) != get<0>(b))
            return get<0>(a) < get<0>(b);
        if (get<2>(a) != get<2>(b))
            return get<2>(a) < get<2>(b);
        if (scheme_order(get<1>(a)) != scheme_order(get<1>(b)))
            return scheme_order(get<1>(a)) < scheme_order(get<1>(b));
        return get<1>(a) < get<1>(b);
    });
    return ordered;
}

static void write_incremental_csv(const string &path, const map<IndexKey, vector<double> > &index)
{
    ofstream output(path.c_str());
    if (!output)
    {
        throw runtime_error("failed to write " + path);
    }

    const string parent = "vhss";
    const vector<tuple<string, string, int> > keys = protocol_keys(index);
    output << "backend,scheme,parent,degree,setup_incremental_ms,gen_incremental_ms,input_incremental_ms,eval_incremental_ms,verify_incremental_ms,decode_incremental_ms\n";
    for (size_t i = 0; i < keys.size(); ++i)
    {
        const string backend = get<0>(keys[i]);
        const string scheme = get<1>(keys[i]);
        const int degree = get<2>(keys[i]);
        output << csv_escape(backend) << ","
               << csv_escape(scheme) << ","
               << csv_escape(parent) << ","
               << degree << ","
               << format_ms(incremental_value(index, backend, scheme, parent, degree, "setup")) << ","
               << format_ms(incremental_value(index, backend, scheme, parent, degree, "gen")) << ","
               << format_ms(incremental_value(index, backend, scheme, parent, degree, "input")) << ","
               << format_ms(incremental_value(index, backend, scheme, parent, degree, "eval")) << ","
               << format_ms(incremental_value(index, backend, scheme, parent, degree, "verify")) << ","
               << format_ms(incremental_value(index, backend, scheme, parent, degree, "decode")) << "\n";
    }
    cout << "Wrote " << keys.size() << " incremental timing rows to " << path << "\n";
}

int main(int argc, char **argv)
{
    try
    {
        string protocol_components = "benchmarks/results/protocols/logs/protocol_components.csv";
        string out_dir = "benchmarks/results/overhead";

        for (int i = 1; i < argc; ++i)
        {
            const string arg(argv[i]);
            if (arg == "--protocol-components" && i + 1 < argc)
            {
                protocol_components = argv[++i];
            }
            else if (arg == "--out-dir" && i + 1 < argc)
            {
                out_dir = argv[++i];
            }
            else if (arg == "--help")
            {
                cout << "Usage: " << argv[0]
                     << " [--protocol-components PATH] [--out-dir DIR]\n";
                return 0;
            }
            else
            {
                cerr << "Usage: " << argv[0]
                     << " [--protocol-components PATH] [--out-dir DIR]\n";
                return 2;
            }
        }

        make_dirs(out_dir);
        const map<IndexKey, vector<double> > index = read_protocol_components(protocol_components);
        write_incremental_csv(join_path(out_dir, "incremental_timing.csv"), index);
    }
    catch (const exception &error)
    {
        cerr << "incremental_timing_generator: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
