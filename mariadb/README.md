# mariaext-analyzer
Python scripts for extension testing analysis for MariaDB. Since MySQL and MariaDB's extensibility ecosystems are remarkably similar, we decided to test them as one unit.

# Dependencies
Here are the Ubuntu packages required to run our scripts:
- mariadb-plugin-connect
- mariadb-plugin-cracklib-password-check
- mariadb-plugin-gssapi-server
- mariadb-plugin-mroonga
- mariadb-plugin-rocksdb
- mariadb-plugin-oqgraph
- mariadb-plugin-s3

Additionally, you need to install the MariaDB Python library with `pip`.

# Scripts
There are two scripts that can be run. First, `source-code-analysis.py` takes no arguments and outputs a CSV file containing source code statistics on MariaDB extensions. This script can be run with default Python
(`python3 source-code-analysis.py`).

In order to run the compatibility script, you will need to set up a MariaDB instance on Ubuntu. Then, you
will type your username and password into the global variables on lines 12-13 of `compatibility-analysis.py`.
This script runs compatibility testing on all pairs of extensions. The output of this script is a CSV file with the compatibility results. It can be run with default Python (`python3 compatibility-analysis.py`).