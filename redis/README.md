# redisext-analyzer
Python scripts for extension testing analysis for Redis.

# Dependencies
The Python packages used in our implementation are:
- csv
- datetime
- json
- os
- re
- subprocess
- urllib

All of these should be automatically included with a Python installation, but if they are not, then they can be installed with `pip`.

# Scripts
There is one script that can be run: `source_code_analysis.py`. This script takes no arguments and outputs a CSV file containing source code statistics on Redis extensions. It can be run with default Python (`python3 source_code_analysis.py`).