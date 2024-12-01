import json
import os
import subprocess

sobbing_lines = open("sobbing.txt", "r").readlines()
json_template = {
  "github": "",
  "test_directories": [],
}

for e in sobbing_lines:
  filename = e.strip() + ".json"
  subprocess.run("touch extn_info/" + filename, shell=True)
  out_file = open("extn_info/" + filename, "w")
  json.dump(json_template, out_file, indent = 2)
  out_file.close()
