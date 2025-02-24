# sqliteext-analyzer
Python scripts for extension testing analysis for SQLite.

# Dependencies
The Python packages used in our implementation are:
- csv
- datetime
- json
- os
- queue
- random
- re
- sqlite3
- subprocess
- threading
- urllib

All of these should be automatically included with a Python installation, but if they are not, then they can be installed with `pip`.

# Scripts
There are two scripts that can be run. First, `source_code_analysis.py` takes no arguments and outputs a CSV file containing source code statistics on SQLite extensions. This script can be run with default Python
(`python3 source_code_analysis.py`).

The `compatibility.py` has one global variable (SINGLE, on line 11 of `compatibility.py`) that sets up the different modes of analysis. The single testing mode (`SINGLE = True`) allows each individual extension to be tested for functionality, while the compatibility testing mode (`SINGLE = False`) runs compatibility testing on all pairs of extensions. The output of this script is a CSV file with the compatibility results.
This script can be run with default Python (`python3 compatibility.py`).