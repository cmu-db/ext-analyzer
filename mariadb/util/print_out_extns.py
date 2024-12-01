import json 
import os

def get_file_extns_list(file_extns_filename):
  f = open(file_extns_filename, "r")
  file_extns_list = f.readlines()
  file_extns_list = list(map(lambda x: x.strip("\n"),file_extns_list))
  file_extns_list = list(map(lambda x: x.strip(),file_extns_list))
  file_extns_list = list(map(lambda x: x.lower(),file_extns_list))
  f.close()
  return file_extns_list

current_working_dir = os.getcwd()
extn_info_dir = "extn_info"
extn_files = os.listdir(current_working_dir + "/" + extn_info_dir)
extn_db = {}
for file in extn_files:
  extn_info_file = open(current_working_dir + "/" + extn_info_dir + "/" + file, "r")
  extn_info_json = json.load(extn_info_file)
  key = os.path.splitext(file)[0]
  extn_db[key] = extn_info_json

sorted_keys = list(extn_db.keys())
sorted_keys.sort()
for key in sorted_keys:
  extn_entry = extn_db[key]
  if extn_entry["type"] == "storage_engine":
    print(key)