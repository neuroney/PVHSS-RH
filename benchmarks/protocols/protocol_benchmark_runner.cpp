#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdio>
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

struct Target
{
    string name;
    string backend;
    string scheme;
    string binary;
};

struct ComponentRow
{
    string backend;
    string scheme;
    string target;
    int degree;
    string phase;
    string label;
    int samples;
    double mean_ms;
    double rsd_percent;
    string log_path;
};

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

static const Target kTargets[] = {
    {"group-dc", "group", "dc", "benchmarks/protocols/protocol_group_dc_bench"},
    {"group-hss", "group", "hss", "benchmarks/protocols/protocol_group_hss_bench"},
    {"group-ot", "group", "ot", "benchmarks/protocols/protocol_group_ot_bench"},
    {"group-vhss", "group", "vhss", "benchmarks/protocols/protocol_group_vhss_bench"},
    {"rlwe-cz", "rlwe", "cz", "benchmarks/protocols/protocol_cz_bench"},
    {"rlwe-dc", "rlwe", "dc", "benchmarks/protocols/protocol_rlwe_dc_bench"},
    {"rlwe-hss", "rlwe", "hss", "benchmarks/protocols/protocol_rlwe_hss_bench"},
    {"rlwe-ot", "rlwe", "ot", "benchmarks/protocols/protocol_rlwe_ot_bench"},
    {"rlwe-vhss", "rlwe", "vhss", "benchmarks/protocols/protocol_rlwe_vhss_bench"},
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

static bool starts_with(const string &value, const string &prefix)
{
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

static bool contains(const string &value, const string &needle)
{
    return value.find(needle) != string::npos;
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

static string shell_quote(const string &value)
{
    string out = "'";
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '\'')
        {
            out += "'\\''";
        }
        else
        {
            out += value[i];
        }
    }
    out += "'";
    return out;
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

static string normalize_phase(const string &label)
{
    const string value = lower_copy(trim(label));
    if (contains(value, "setup base"))
        return "SetupBase";
    if (contains(value, "setup incremental"))
        return "SetupIncremental";
    if (contains(value, "keygen incremental") || contains(value, "gen incremental"))
        return "GenIncremental";
    if (contains(value, "evaluation base 0"))
        return "EvalBase0";
    if (contains(value, "evaluation base 1"))
        return "EvalBase1";
    if (contains(value, "evaluation incremental 0"))
        return "EvalIncremental0";
    if (contains(value, "evaluation incremental 1"))
        return "EvalIncremental1";
    if (contains(value, "prove"))
        return "EvalIncremental";
    if (contains(value, "evaluation 0") || contains(value, "evaluate0") || contains(value, "server 1"))
        return "Eval0";
    if (contains(value, "evaluation 1") || contains(value, "evaluate1") || contains(value, "server 2"))
        return "Eval1";
    if (contains(value, "eval"))
        return "Eval";
    if (contains(value, "verification") || starts_with(value, "ver "))
        return "Verify";
    if (contains(value, "decryption") || contains(value, "decoding") || starts_with(value, "dec "))
        return "Decode";
    if (contains(value, "keygen") || contains(value, "key gen"))
        return "Gen";
    if (starts_with(value, "gen ") || contains(value, "hss_gen") || contains(value, " gen "))
        return "Gen";
    if (contains(value, "enc time setup") || contains(value, "input") || contains(value, "hss_input"))
        return "Input";
    if (starts_with(value, "enc "))
        return "Input";
    if (contains(value, "setup"))
        return "Setup";
    if (contains(value, "output"))
        return "Decode";
    return trim(label);
}

static int scheme_order(const string &scheme)
{
    if (scheme == "hss")
        return 0;
    if (scheme == "vhss")
        return 1;
    if (scheme == "ot")
        return 2;
    if (scheme == "dc")
        return 3;
    if (scheme == "cz")
        return 4;
    return 99;
}

static const Target *find_target(const string &name)
{
    for (size_t i = 0; i < sizeof(kTargets) / sizeof(kTargets[0]); ++i)
    {
        if (kTargets[i].name == name)
        {
            return &kTargets[i];
        }
    }
    return NULL;
}

static bool parse_degree_line(const string &line, int &degree)
{
    const string value = trim(line);
    if (!starts_with(value, "degree_f") && !starts_with(value, "Degree"))
    {
        return false;
    }
    const size_t colon = value.find(':');
    if (colon == string::npos)
    {
        return false;
    }
    degree = atoi(trim(value.substr(colon + 1)).c_str());
    return degree > 0;
}

static bool parse_timing_line(const string &line, string &label, double &mean_ms, double &rsd_percent)
{
    const size_t colon = line.find(':');
    if (colon == string::npos)
    {
        return false;
    }

    label = trim(line.substr(0, colon));
    const string rest = trim(line.substr(colon + 1));
    char *end = NULL;
    mean_ms = strtod(rest.c_str(), &end);
    if (end == rest.c_str())
    {
        return false;
    }

    const string after_mean = trim(string(end));
    if (!starts_with(after_mean, "ms"))
    {
        return false;
    }

    const size_t rsd_pos = after_mean.find("RSD:");
    if (rsd_pos == string::npos)
    {
        return false;
    }

    const string rsd_text = trim(after_mean.substr(rsd_pos + 4));
    rsd_percent = strtod(rsd_text.c_str(), &end);
    return end != rsd_text.c_str();
}

static vector<ComponentRow> parse_log(const string &text, const Target &target,
                                      int samples, const string &log_path)
{
    vector<ComponentRow> rows;
    int degree = -1;
    istringstream input(text);
    string line;
    while (getline(input, line))
    {
        int parsed_degree = 0;
        if (parse_degree_line(line, parsed_degree))
        {
            degree = parsed_degree;
            continue;
        }

        string label;
        double mean_ms = 0.0;
        double rsd_percent = 0.0;
        if (degree < 0 || !parse_timing_line(trim(line), label, mean_ms, rsd_percent))
        {
            continue;
        }

        ComponentRow row;
        row.backend = target.backend;
        row.scheme = target.scheme;
        row.target = target.name;
        row.degree = degree;
        row.phase = normalize_phase(label);
        row.label = label;
        row.samples = samples;
        row.mean_ms = mean_ms;
        row.rsd_percent = rsd_percent;
        row.log_path = log_path;
        rows.push_back(row);
    }
    return rows;
}

static pair<int, string> run_command(const vector<string> &argv)
{
    if (argv.empty())
    {
        throw runtime_error("empty command");
    }

    string command;
    for (size_t i = 0; i < argv.size(); ++i)
    {
        if (i != 0)
        {
            command += " ";
        }
        command += shell_quote(argv[i]);
    }
    command += " 2>&1";

    FILE *pipe = popen(command.c_str(), "r");
    if (pipe == NULL)
    {
        throw runtime_error("popen failed for " + argv[0] + ": " + strerror(errno));
    }

    string output;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL)
    {
        output += buffer;
    }

    const int status = pclose(pipe);
    int exit_code = status;
    if (WIFEXITED(status))
    {
        exit_code = WEXITSTATUS(status);
    }
    return make_pair(exit_code, output);
}

static void write_text_file(const string &path, const string &text)
{
    ofstream output(path.c_str());
    if (!output)
    {
        throw runtime_error("failed to write " + path);
    }
    output << text;
}

static void write_components_csv(const string &path, const vector<ComponentRow> &rows)
{
    ofstream output(path.c_str());
    if (!output)
    {
        throw runtime_error("failed to write " + path);
    }

    output << "backend,scheme,target,degree,phase,label,samples,mean_ms,rsd_percent,log_path\n";
    for (size_t i = 0; i < rows.size(); ++i)
    {
        output << csv_escape(rows[i].backend) << ","
               << csv_escape(rows[i].scheme) << ","
               << csv_escape(rows[i].target) << ","
               << rows[i].degree << ","
               << csv_escape(rows[i].phase) << ","
               << csv_escape(rows[i].label) << ","
               << rows[i].samples << ","
               << fixed << setprecision(6) << rows[i].mean_ms << ","
               << fixed << setprecision(6) << rows[i].rsd_percent << ","
               << csv_escape(rows[i].log_path) << "\n";
    }
}

static map<IndexKey, vector<double> > index_rows(const vector<ComponentRow> &rows)
{
    map<IndexKey, vector<double> > index;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        IndexKey key;
        key.backend = lower_copy(trim(rows[i].backend));
        key.scheme = lower_copy(trim(rows[i].scheme));
        key.degree = rows[i].degree;
        key.phase = trim(rows[i].phase);
        index[key].push_back(rows[i].mean_ms);
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

static void write_timing_csv(const string &path, const vector<ComponentRow> &rows)
{
    const map<IndexKey, vector<double> > index = index_rows(rows);
    set<tuple<string, string, int> > keys;
    for (map<IndexKey, vector<double> >::const_iterator it = index.begin(); it != index.end(); ++it)
    {
        keys.insert(make_tuple(it->first.backend, it->first.scheme, it->first.degree));
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

    ofstream output(path.c_str());
    if (!output)
    {
        throw runtime_error("failed to write " + path);
    }

    output << "backend,scheme,degree,setup_ms,gen_ms,input_ms,eval_ms,verify_ms,decode_ms\n";
    for (size_t i = 0; i < ordered.size(); ++i)
    {
        const string backend = get<0>(ordered[i]);
        const string scheme = get<1>(ordered[i]);
        const int degree = get<2>(ordered[i]);
        output << csv_escape(backend) << ","
               << csv_escape(scheme) << ","
               << degree << ","
               << format_ms(setup_total(index, backend, scheme, degree)) << ","
               << format_ms(gen_total(index, backend, scheme, degree)) << ","
               << format_ms(mean_phase(index, backend, scheme, degree, "Input")) << ","
               << format_ms(eval_total(index, backend, scheme, degree)) << ","
               << format_ms(mean_phase(index, backend, scheme, degree, "Verify")) << ","
               << format_ms(mean_phase(index, backend, scheme, degree, "Decode")) << "\n";
    }
}

static void print_usage(const char *program)
{
    cerr << "Usage: " << program
         << " [--build-dir DIR] [--out-dir DIR] [--degrees LIST] [--samples N]"
         << " [--msg-num N] [--targets NAME...]\n";
}

int main(int argc, char **argv)
{
    try
    {
        string build_dir = "build";
        string out_dir = "benchmarks/results/protocols";
        string degrees = "5,10,15";
        int samples = 1;
        int msg_num = 5;
        bool list_only = false;
        bool dry_run = false;
        vector<string> target_names;

        for (int i = 1; i < argc; ++i)
        {
            const string arg(argv[i]);
            if (arg == "--build-dir" && i + 1 < argc)
            {
                build_dir = argv[++i];
            }
            else if (arg == "--out-dir" && i + 1 < argc)
            {
                out_dir = argv[++i];
            }
            else if (arg == "--degrees" && i + 1 < argc)
            {
                degrees = argv[++i];
            }
            else if (arg == "--samples" && i + 1 < argc)
            {
                samples = max(1, atoi(argv[++i]));
            }
            else if (arg == "--msg-num" && i + 1 < argc)
            {
                msg_num = max(1, atoi(argv[++i]));
            }
            else if (arg == "--targets")
            {
                while (i + 1 < argc && !starts_with(argv[i + 1], "--"))
                {
                    target_names.push_back(argv[++i]);
                }
            }
            else if (arg == "--list")
            {
                list_only = true;
            }
            else if (arg == "--dry-run")
            {
                dry_run = true;
            }
            else if (arg == "--help")
            {
                print_usage(argv[0]);
                return 0;
            }
            else
            {
                print_usage(argv[0]);
                return 2;
            }
        }

        if (list_only)
        {
            for (size_t i = 0; i < sizeof(kTargets) / sizeof(kTargets[0]); ++i)
            {
                cout << kTargets[i].name << ": backend=" << kTargets[i].backend
                     << " scheme=" << kTargets[i].scheme
                     << " binary=" << kTargets[i].binary << "\n";
            }
            return 0;
        }

        if (target_names.empty())
        {
            for (size_t i = 0; i < sizeof(kTargets) / sizeof(kTargets[0]); ++i)
            {
                target_names.push_back(kTargets[i].name);
            }
        }

        const string log_dir = join_path(out_dir, "logs");
        const string component_csv_path = join_path(log_dir, "protocol_components.csv");
        const string timing_csv_path = join_path(out_dir, "protocol_timing.csv");
        vector<ComponentRow> rows;

        if (!dry_run)
        {
            make_dirs(log_dir);
        }

        for (size_t i = 0; i < target_names.size(); ++i)
        {
            const Target *target = find_target(target_names[i]);
            if (target == NULL)
            {
                throw runtime_error("unknown protocol benchmark target: " + target_names[i]);
            }

            const string binary = join_path(build_dir, target->binary);
            vector<string> command;
            command.push_back(binary);
            command.push_back("--msg-num");
            command.push_back(to_string(msg_num));
            command.push_back("--samples");
            command.push_back(to_string(samples));
            command.push_back("--degrees");
            command.push_back(degrees);

            if (dry_run)
            {
                for (size_t j = 0; j < command.size(); ++j)
                {
                    if (j != 0)
                        cout << " ";
                    cout << shell_quote(command[j]);
                }
                cout << "\n";
                continue;
            }

            if (access(binary.c_str(), X_OK) != 0)
            {
                throw runtime_error("missing benchmark binary: " + binary);
            }

            const pair<int, string> completed = run_command(command);
            const string log_path = join_path(log_dir, target->name + ".log");
            write_text_file(log_path, completed.second);
            if (completed.first != 0)
            {
                throw runtime_error("benchmark failed: " + binary + " exit " + to_string(completed.first));
            }

            const vector<ComponentRow> parsed = parse_log(completed.second, *target, samples, log_path);
            rows.insert(rows.end(), parsed.begin(), parsed.end());
        }

        if (dry_run)
        {
            return 0;
        }

        write_timing_csv(timing_csv_path, rows);
        write_components_csv(component_csv_path, rows);

        const map<IndexKey, vector<double> > index = index_rows(rows);
        set<tuple<string, string, int> > keys;
        for (map<IndexKey, vector<double> >::const_iterator it = index.begin(); it != index.end(); ++it)
        {
            keys.insert(make_tuple(it->first.backend, it->first.scheme, it->first.degree));
        }
        cout << "Wrote " << keys.size() << " protocol timing rows to " << timing_csv_path << "\n";
        cout << "Wrote " << rows.size() << " internal component rows to " << component_csv_path << "\n";
    }
    catch (const exception &error)
    {
        cerr << "protocol_benchmark_runner: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
