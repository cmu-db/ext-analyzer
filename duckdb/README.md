# duckext-analyzer
Python scripts for extension testing analysis for DuckDB.

# Dependencies
Here are the Ubuntu packages required to run our scripts:
- libgfortran5

Additionally, you need to install the DuckDB Python library with `pip`.

# Scripts
There are two scripts that can be run. First, `source-code-analysis.py` takes no arguments and outputs a CSV file containing source code statistics on DuckDB extensions. This script can be run with default Python
(`python3 source-code-analysis.py`).

`compatibility_analysis.py` is a draft of a script for DuckDB extension compatibility testing. It currently
does not work. We have included it since it still may be useful reading material. 

The script that works is `compatibility_v2.py`. It can be run with default Python (`python3 compatibility_v2.py`). The output of this script is a CSV file with the compatibility results.