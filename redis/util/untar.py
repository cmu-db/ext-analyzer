import os
import subprocess

current_working_dir = os.getcwd()
extn_src_code = "extn_src_code"

stuff = os.listdir(current_working_dir + "/" + extn_src_code)
for elem in stuff:
  if elem.endswith(".tar.gz"):
    subprocess.run("tar -xvf " + elem, shell=True, cwd=current_working_dir + "/" + extn_src_code)
  elif elem.endswith(".zip"):
    subprocess.run("unzip " + elem, shell=True, cwd=current_working_dir + "/" + extn_src_code)