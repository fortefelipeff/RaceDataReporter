# RaceDataReporter

RaceDataReporter is a C++ console application that reads MoTeC-style CSV telemetry files from race cars and generates rich HTML reports. Each report summarizes key performance indicators (KPIs) with tables (mean, minimum and maximum for each channel) and a series of interactive scatter plots built with Chart.js.

## Features

- **Automatic CSV parsing:** Detects and skips header metadata and unit rows exported by MoTeC.
- **Statistics for every channel:** Calculates mean, minimum and maximum for all data channels in the CSV.
- **Interactive plots:** Generates separate scatter plots comparing each channel against corrected speed (`Corr Speed`). Plots are downsampled automatically for large datasets to keep the page responsive.
- **Self-contained HTML report:** Embeds Chart.js from a CDN and styles tables for readability. Simply open the report in a browser to explore the data.

## Example workflow

1. Place your CSV file (e.g., `LMP3 Ref_Barcelona_v2.csv`) in the same directory as the application.
2. Build and run the program:
   ```bash
   g++ -std=c++17 -o RaceDataReporter RaceDataReporter.cpp
   ./RaceDataReporter
   ```
   On Windows/Visual Studio, create a new **Console App** project, add `RaceDataReporter.cpp` to it, ensure the CSV filename matches in the code (`CSV_FILENAME`), then build and run.
3. The program reads the CSV, computes KPI statistics and writes `report.html`.
4. Open `report.html` in your favourite browser to view the tables and plots.

## Dependencies

RaceDataReporter uses only the C++ standard library; no external libraries are required for the backend. The generated HTML imports Chart.js via CDN for plotting.

## Repository contents

- `RaceDataReporter.cpp` – the C++ source code for the application.
- `LMP3 Ref_Barcelona_v2.csv` – sample telemetry data (for demonstration).
- `report_preview.html` – sample HTML report generated from the provided CSV.

## License

This project is provided for educational and portfolio purposes. Feel free to fork and adapt it for your own telemetry analysis.
