#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

static const std::string CSV_FILENAME = "LMP3 Ref_Barcelona_v2.csv";

struct ChannelStats {
    double sum = 0.0;
    double min = std::numeric_limits<double>::infinity();
    double max = -std::numeric_limits<double>::infinity();
    std::vector<double> values;
};

static std::string trim(const std::string &s) {
    const char *whitespace = " \t\n\r\f\v";
    size_t start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

static std::vector<std::string> parseCsvLine(const std::string &line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(trim(field));
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(trim(field));
    return fields;
}

int main() {
    std::ifstream csv(CSV_FILENAME);
    if (!csv.is_open()) {
        std::cerr << "Erro: não foi possível abrir o arquivo CSV '" << CSV_FILENAME << "'.\n";
        return 1;
    }

    std::string line;
    std::vector<std::string> header;
    std::unordered_map<std::string, ChannelStats> channels;
    std::vector<double> corrSpeed;

    auto stripBOM = [](std::string &s) {
        if (s.size() >= 3 &&
            static_cast<unsigned char>(s[0]) == 0xEF &&
            static_cast<unsigned char>(s[1]) == 0xBB &&
            static_cast<unsigned char>(s[2]) == 0xBF) {
            s.erase(0, 3);
        }
    };

    while (std::getline(csv, line)) {
        stripBOM(line);
        auto cells = parseCsvLine(line);
        if (!cells.empty()) {
            std::string first = cells[0];
            if (!first.empty() && first.front() == '"' && first.back() == '"')
                first = first.substr(1, first.size() - 2);
            bool hasCorrSpeed = std::find(cells.begin(), cells.end(), "Corr Speed") != cells.end();
            if ((first == "Time") && hasCorrSpeed) {
                header = cells;
                break;
            }
        }
    }

    if (header.empty()) {
        std::cerr << "Erro: cabeçalho não encontrado no arquivo CSV.\n";
        return 1;
    }

    while (std::getline(csv, line)) {
        auto cells = parseCsvLine(line);
        if (!cells.empty() && cells.size() == header.size()) {
            std::string first = cells[0];
            if (!first.empty() && first.front() == '"' && first.back() == '"')
                first = first.substr(1, first.size() - 2);
            if (first != "Time") {
                break;
            }
        }
    }

    for (const auto &name : header) {
        if (name == "Time" || name.empty()) continue;
        channels[name] = ChannelStats{};
    }

    while (std::getline(csv, line)) {
        if (line.empty()) continue;
        auto fields = parseCsvLine(line);
        if (fields.size() != header.size()) continue;

        for (size_t i = 0; i < header.size(); ++i) {
            const std::string &name = header[i];
            if (name == "Time" || name.empty()) continue;
            const std::string &valueStr = fields[i];
            if (valueStr.empty()) continue;
            try {
                double value = std::stod(valueStr);
                auto &stat = channels[name];
                stat.values.push_back(value);
                stat.sum += value;
                if (value < stat.min) stat.min = value;
                if (value > stat.max) stat.max = value;
            } catch (...) {}
        }

        auto it = std::find(header.begin(), header.end(), "Corr Speed");
        if (it != header.end()) {
            size_t idx = std::distance(header.begin(), it);
            try {
                double speed = std::stod(fields[idx]);
                corrSpeed.push_back(speed);
            } catch (...) {
                corrSpeed.push_back(0.0);
            }
        }
    }
    csv.close();

    std::unordered_map<std::string, double> means;
    for (auto &kv : channels) {
        const std::string &name = kv.first;
        ChannelStats &stat = kv.second;
        if (!stat.values.empty()) {
            means[name] = stat.sum / static_cast<double>(stat.values.size());
        } else {
            means[name] = 0.0;
        }
    }

    std::ofstream html("report.html");
    html << "<!DOCTYPE html><html lang=\"pt\"><head><meta charset=\"UTF-8\">";
    html << "<title>Relatório de Dados de Corrida</title>";
    html << "<style>"
         << "body{font-family:Arial,sans-serif;margin:20px;}"
         << "table{border-collapse:collapse;width:100%;margin-bottom:40px;}"
         << "th,td{border:1px solid #ddd;padding:8px;text-align:right;}"
         << "th{text-align:center;background-color:#f2f2f2;}"
         << "tr:nth-child(even){background-color:#f9f9f9;}"
         << ".chart-wrapper{width:100%;height:300px;margin-bottom:40px;position:relative;}"
         << ".chart-wrapper canvas{width:100%!important;height:100%!important;}"
         << "</style>";
    html << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>";
    html << "</head><body>";
    html << "<h1>Relatório de Dados de Corrida</h1>";

    html << "<h2>Resumo Estatístico</h2><table><tr><th>Canal</th><th>Média</th><th>Mínimo</th><th>Máximo</th></tr>";
    for (const auto &name : header) {
        if (name == "Time" || name.empty()) continue;
        const auto &stat = channels[name];
        html << "<tr><td style=\"text-align:left;\">" << name << "</td>"
             << std::fixed << std::setprecision(2)
             << "<td>" << means[name] << "</td>"
             << "<td>" << stat.min << "</td>"
             << "<td>" << stat.max << "</td></tr>";
    }
    html << "</table>";

    html << "<h2>Gráficos: cada canal vs Corr Speed</h2>";

    for (const auto &name : header) {
        if (name == "Time" || name == "Corr Speed" || name.empty()) continue;
        std::string sanitized = name;
        for (char &c : sanitized) {
            if (!std::isalnum(static_cast<unsigned char>(c))) c = '_';
        }
        html << "<div class=\"chart-wrapper\">"
             << "<h3>" << name << " vs Corr Speed</h3>"
             << "<canvas id=\"" << sanitized << "Chart\"></canvas>"
             << "</div>";
    }

    html << "<script>";
    html << "const corrSpeed=[";
    for (size_t i=0; i<corrSpeed.size(); ++i) {
        html << corrSpeed[i];
        if (i+1 < corrSpeed.size()) html << ",";
    }
    html << "];";
    html << "const colors=['rgba(255,99,132,0.8)','rgba(54,162,235,0.8)','rgba(255,206,86,0.8)','rgba(75,192,192,0.8)',"
         "'rgba(153,102,255,0.8)','rgba(255,159,64,0.8)','rgba(199,199,199,0.8)','rgba(83,102,255,0.8)',"
         "'rgba(255,99,71,0.8)','rgba(60,179,113,0.8)','rgba(238,130,238,0.8)','rgba(30,144,255,0.8)',"
         "'rgba(255,215,0,0.8)','rgba(255,105,180,0.8)','rgba(0,206,209,0.8)'];";
    int colorIndexLocal = 0;
    for (const auto &name : header) {
        if (name == "Time" || name == "Corr Speed" || name.empty()) continue;
        std::string sanitized = name;
        for (char &c : sanitized) {
            if (!std::isalnum(static_cast<unsigned char>(c))) c = '_';
        }
        const auto &vals = channels[name].values;
        size_t count = std::min(vals.size(), corrSpeed.size());
        size_t step = 1;
        if (count > 1000) step = count / 1000;
        html << "const data_" << sanitized << "=[";
        bool first = true;
        for (size_t i=0; i<count; i += step) {
            if (!first) html << ",";
            html << vals[i];
            first = false;
        }
        html << "];";
        html << "{"
             << "const ctx=document.getElementById('" << sanitized << "Chart').getContext('2d');"
             << "const pts=data_" << sanitized << ".map((y,i)=>({x:corrSpeed[i*" << step << "],y:y}));"
             << "new Chart(ctx,{type:'scatter',data:{datasets:[{label:'" << name << "',data:pts,"
             << "showLine:false,borderColor:colors[" << (colorIndexLocal % 15) << "],pointRadius:2}]},"
             << "options:{plugins:{legend:{display:false}},"
             << "scales:{x:{title:{display:true,text:'Corr Speed (km/h)'}},"
             << "y:{title:{display:true,text:'" << name << "'}}},responsive:true,maintainAspectRatio:false}});"
             << "}";
        ++colorIndexLocal;
    }
    html << "</script>";
    html << "</body></html>";
    html.close();

    std::cout << "Relatório gerado com sucesso.\n";
    return 0;
}
