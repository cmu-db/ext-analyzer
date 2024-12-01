import os
import json

current_working_dir = os.getcwd()
extn_info_dir = "extn_info"
ext_work_dir = "duckextworkdir"

# Load extension database
extn_files = os.listdir(current_working_dir + "/" + extn_info_dir)
extn_db = {}
for file in extn_files:
  extn_info_file = open(current_working_dir + "/" + extn_info_dir + "/" + file, "r")
  extn_info_json = json.load(extn_info_file)
  key = os.path.splitext(file)[0]
  extn_db[key] = extn_info_json

extns = list(extn_db.keys())
extns.sort()

for extn in extns:
  extn_entry = extn_db[extn]
  folder_name = extn_entry["folder_name"]
  if "core_folder" not in extn_entry:
    dirs = os.listdir(current_working_dir + "/" + ext_work_dir + "/" + folder_name)
    extn_filename = extn + "_extension.cpp"
    if extn_filename in dirs:
      print(extn)