# Extension Analyzer

This repository contains the code used to analyze the extension ecosystems of PostgreSQL, MySQL/MariaDB, SQLite, Redis, and DuckDB. The code for each of these systems and the instructions on how to run the analysis scripts can be found in their respective directories, except the PostgreSQL index-type analysis, which is included as a submodule.

Additionally, we have included our the CSVs used to run our plot scripts and the plot scripts themselves in the `plot_scripts` directory.

Feel free to submit PRs and offer feedback, we would love to hear it!

# Specifications
- This code runs on a machine with Ubuntu 22.04. Compatibility with other OSes is not supported.
- All Python scripts should work with Python version 3.12.

This repository includes submodules. To clone it, you need to use the `--recurse-submodules` option.