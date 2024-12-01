# dependencies
- mariadb python library
- mariadb-plugin-connect
- mariadb-plugin-cracklib-password-check
- mariadb-plugin-gssapi-server
- mariadb-plugin-mroonga
- mariadb-plugin-rocksdb
- mariadb-plugin-oqgraph
- mariadb-plugin-s3

I could support an install list or something but instead I just killed the "so_name": "auth_gssapi" from gssapi.json :) academic code rocks

Storage engines I did not test because it was 2am and they required extra set up and I wanted preliminary results
- federated
- federatedx
- s3
- oqgraph
- sphinx
- mrg_myisam
- spider (distributed)


no source: memory, s3

things that i don't know where source is:
- password_reuse_check
- metadata_lock_info
- auth_pipe