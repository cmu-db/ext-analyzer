import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.ticker as ticker
from matplotlib.ticker import MultipleLocator

# Globals
COLOR = "#38a9c5"
TURQUOISE_COLORS = ["#38a9c5", "#1b8da7"]
EDGECOLOR = "black"
LINE_WEIGHT = 0.5
SMALL_SIZE = 16
MEDIUM_SIZE = 20
FIG_SIZE = (10, 4)

import matplotlib.font_manager as font_manager

# Importing Futura Light
filename = "/Users/abigale/Downloads/Futura-Light.ttf"
custom_font = font_manager.FontEntry(fname=filename, name="Futura Light")
font_manager.fontManager.ttflist.insert(0, custom_font)
plt.rcParams['font.family'] = "Futura"

## Setting MPL params
plt.rc('font', size=14) 
plt.rc('axes', titlesize=SMALL_SIZE) 
plt.rc('axes', labelsize=MEDIUM_SIZE) 
plt.rc('xtick', labelsize=SMALL_SIZE)
plt.rc('ytick', labelsize=SMALL_SIZE) 

### Helper Functions ###
def get_pd_df_from_file(filename):
  file_path = "csvs/" + filename + ".csv"
  data = pd.read_csv(file_path)
  return data

def make_histogram(filename, bin_list, x_label_name, y_label_name, x_tick_list=None, y_tick_list=None, figsize=None):
  values = get_pd_df_from_file(filename)['data']

  fig, ax = plt.subplots(layout='constrained')
  ax.yaxis.grid(True, color="gray")
  ax.set_axisbelow(True)
  fig.set_size_inches(10, 4)
  color_arr = []
  for i in range(len(bin_list)):
    color_arr.append(TURQUOISE_COLORS[i % 2])

  # Calculate histogram data manually
  counts, bin_edges = np.histogram(values, bins=bin_list)

  # Create the bar plot with custom colors
  bars = plt.bar(bin_edges[:-1], counts, width=np.diff(bin_edges), 
          color=color_arr, 
          edgecolor=EDGECOLOR, 
          align='edge')

  ax.bar_label(bars)

  #plt.hist(values, bins=bin_list, color=color_arr, edgecolor=EDGECOLOR, lw=LINE_WEIGHT)
  plt.xlabel(x_label_name)
  plt.ylabel(y_label_name)
  
  if x_tick_list != None:
    plt.xticks(x_tick_list)
  else:
    plt.xticks()

  if y_tick_list != None:
    plt.yticks(y_tick_list)
  else:
    plt.yticks()

 #plt.subplots_adjust(bottom=0.15, top=0.95)
  plt.savefig(filename + "_hist.pdf")

def multi_line_string(label : str, line_length):
  str_list = label.split()
  new_str = ""
  ll = 0
  for elem in str_list:
    if ll + len(elem) < line_length:
      ll += len(elem) + 1
      new_str += elem + " "
    else:
      new_str += "\n"
      new_str += elem + " "
      ll = len(elem) + 1
  return new_str.strip()


def make_bar_chart(filename, x_label_name, y_label_name, x_tick_list=None, y_tick_list=None, width=0.8, rotation=None):
  data = get_pd_df_from_file(filename)
  labels = data['labels']

  if type(labels[0]) == str:
    labels = list(map(lambda x : multi_line_string(x, 13), labels))
  values = data['values']

  plt.figure(figsize=FIG_SIZE)
  plt.bar(labels, values, color=COLOR, edgecolor=EDGECOLOR, lw=LINE_WEIGHT, width=width)
  plt.xlabel(x_label_name)
  plt.ylabel(y_label_name)
  plt.xticks(ticks=x_tick_list, rotation=rotation)

  plt.subplots_adjust(bottom=0.15)
  plt.savefig(filename + "_bar.pdf")

def compatibility_plot():
  bin_list = []
  for i in range(0, 20):
    bin_list.append(i * 5)

  tick_list = []
  for i in range(0, 10):
    tick_list.append(i * 10)

  make_histogram("compatibility", bin_list, "Percentage of Compatibility Test Failures", "Number of Extensions", x_tick_list=tick_list)

def postgresql_plot():
  bin_list = []
  for i in range(0, 9):
    bin_list.append(i * 10)
  
  make_histogram("postgresql", bin_list, "Percentage of Copied PostgreSQL Code in Codebase", "Number of Extensions", x_tick_list=bin_list, y_tick_list=[0, 10, 20, 30, 40, 50])

def versioning_plot():
  bin_list = []
  for i in range(0, 8):
    bin_list.append(i * 10)

  make_histogram("versioning", bin_list, "Percentage of Encapsulated Versioning Code in Codebase", "Number of Extensions", x_tick_list=bin_list, y_tick_list=[0, 20, 40, 60, 80, 100, 120, 140])

def func_plot():
  bin_list = []
  for i in range(0, 11):
    bin_list.append(i * 10)
  
  make_histogram("func", bin_list, "Percentage of LOC Consisting of Whole Functions in Copied Code", "Number of Extensions", x_tick_list=bin_list, y_tick_list=[0,1,2,3,4,5,6,7], figsize=(10,6))

def number_of_extns_using_type(info_df, t, num):
  new_df = info_df[info_df[t] == "Yes"][info_df["Number of Components"] == num]
  return new_df.shape[0]

def num_components_plot_alternate():
  info_data = get_pd_df_from_file("infos")
  fig, ax = plt.subplots(layout='constrained')

  def forward(ticks):
    _t = np.copy(ticks)
    ticks = np.array(ticks)
    ticks = np.where(ticks == 0, 0.0, ticks)
    ticks = np.where(ticks <= 10, 0.33*(ticks/10), ticks)
    ticks = np.where(ticks > 10, 0.33 + 0.67*((ticks-10)/190), ticks)
    return ticks
  
  def inverse(ticks):
    _t = np.copy(ticks)
    ticks = np.array(ticks)
    ticks = np.where(ticks == 0, 0.0, ticks)
    ticks = np.where(ticks <= 0.33, ticks*10/0.33, ticks)
    ticks = np.where(ticks > 0.33, 10+190*(ticks-0.33)/0.67, ticks)
    return ticks
  
  ax.yaxis.grid(True, color="gray")
  ax.set_axisbelow(True)
  ax.set_yscale("function", functions=(forward, inverse))
  y_labels = [0, 5, 10, 50, 100, 150, 200]
  ax.set_yticks(y_labels)

  d = 0.25  # proportion of vertical to horizontal extent of the slanted line
  kwargs = dict(marker=[(-1, -d), (1, d)], markersize=12, linestyle="none", color='k', mec='k', mew=1, clip_on=False)
  ax.plot([0], [0.34], transform=ax.transAxes, **kwargs)
  ax.plot([0], [0.36], transform=ax.transAxes, **kwargs)


  labels = [1, 2, 3, 4, 5, 6]
  values =  []
  for l in labels:
    val = info_data[info_data["Number of Components"] == l].shape[0]
    values.append(val)
  
  colors = ['#f565cc', '#f77189', '#dc8932', '#ffd966', '#77ab31', '#6e9bf4', '#cc7af4']

  fig.set_size_inches(FIG_SIZE)
  ax.bar(labels, values, color="#f2eddc", edgecolor=EDGECOLOR, lw=LINE_WEIGHT, width=0.77)
  ax.set_xlabel("Number of Extensibility Types", fontsize=20)
  ax.set_ylabel("Number of Extensions", fontsize=20)
  

  print(info_data)
  types = ["Functions",
    "Types",
    "Index Access Methods",
    "Storage Managers",
    "Client Authentication",
    "Query Processing",
    "Utility Commands"]
  
  num_components_dict = {}
  for t in types:
    print(t)
    num_components_dict[t] = []

  for i in range(1,7):
    for t in types:
      num_components_dict[t].append(number_of_extns_using_type(info_data, t, i))
  print(num_components_dict)

  x = np.array([1, 2, 3, 4, 5, 6])
  
  multiplier = 0
  for attribute, measurements in num_components_dict.items():
    offset = 0.11 * multiplier
    print(multiplier)
    _rects1 = ax.bar(x + offset - 0.33, measurements, 0.11, label=attribute, edgecolor=EDGECOLOR, color=colors[multiplier], lw=LINE_WEIGHT)
    #@ax.bar_label(rects, padding=3)
    multiplier += 1

  ax.legend(loc='upper right', ncols=2)
  plt.savefig("num_components_bar.pdf")

def num_components_plot():
  info_data = get_pd_df_from_file("infos")
  fig, ax = plt.subplots(layout='constrained')
  ax.yaxis.grid(True, color="gray")
  ax.set_axisbelow(True)

  labels = [1, 2, 3, 4, 5, 6]
  values =  []
  for l in labels:
    val = info_data[info_data["Number of Components"] == l].shape[0]
    values.append(val)
  
  colors = ['#f565cc', '#f77189', '#dc8932', '#ffd966', '#77ab31', '#6e9bf4', '#cc7af4']

  fig.set_size_inches(FIG_SIZE)
  ax.bar(labels, values, color="#f2eddc", edgecolor=EDGECOLOR, lw=LINE_WEIGHT, width=0.77)
  ax.set_xlabel("Number of Extensibility Types", fontsize=20)
  ax.set_ylabel("Number of Extensions", fontsize=20)
  

  print(info_data)
  types = ["Functions",
    "Types",
    "Index Access Methods",
    "Storage Managers",
    "Client Authentication",
    "Query Processing",
    "Utility Commands"]
  
  num_components_dict = {}
  for t in types:
    print(t)
    num_components_dict[t] = []

  for i in range(1,7):
    for t in types:
      num_components_dict[t].append(number_of_extns_using_type(info_data, t, i))
  print(num_components_dict)

  x = np.array([1, 2, 3, 4, 5, 6])
  
  multiplier = 0
  for attribute, measurements in num_components_dict.items():
    offset = 0.11 * multiplier
    print(multiplier)
    _rects1 = ax.bar(x + offset - 0.33, measurements, 0.11, label=attribute, edgecolor=EDGECOLOR, color=colors[multiplier], lw=LINE_WEIGHT)
    #@ax.bar_label(rects, padding=3)
    multiplier += 1

  ax.legend(loc='upper right', ncols=2)
  #plt.show()
  plt.savefig("num_components_bar.pdf")

def type_components_plot():
  make_bar_chart("type_components", "Type of Extensibility", "Number of Extensions", width=0.5, rotation=45)

def num_system_components_plot():
  mechanisms_data = get_pd_df_from_file("mechanisms")
  fig, ax = plt.subplots(layout='constrained')
  ax.set_axisbelow(True)

  labels = [0, 1, 2, 3]
  values =  []
  for l in labels:
    val = mechanisms_data[mechanisms_data["Number of Components"] == l].shape[0]
    values.append(val)
  
  colors = ['#dc8932', '#77ab31', '#cc7af4']

  fig.set_size_inches(FIG_SIZE)
  ax.yaxis.grid(True, color="gray")
  ax.bar(labels, values, color="#f2eddc", edgecolor=EDGECOLOR, lw=LINE_WEIGHT, width=0.78)
  ax.set_xlabel("Number of System Components", fontsize=20)
  ax.set_ylabel("Number of Extensions", fontsize=20)
  ax.set_xticks(labels)

  types = ["Memory Allocation",
           "Background Workers",
           "Custom Configuration Variables"]
  
  num_components_dict = {}
  for t in types:
    print(t)
    num_components_dict[t] = []

  for i in labels:
    for t in types:
      num_components_dict[t].append(number_of_extns_using_type(mechanisms_data, t, i))
  print(num_components_dict)

  x = np.array(labels)
  
  multiplier = 0
  for attribute, measurements in num_components_dict.items():
    offset = 0.26 * multiplier
    print(multiplier)
    _rects = ax.bar(x + offset - 0.26, measurements, 0.26, label=attribute, edgecolor=EDGECOLOR, color=colors[multiplier], lw=LINE_WEIGHT)
    #@ax.bar_label(rects, padding=3)
    multiplier += 1

  ax.legend(loc='upper right', ncols=3)
  #plt.show()
  plt.savefig("num_system_components_bar.pdf")

if __name__ == '__main__':
  compatibility_plot()
  postgresql_plot()
  versioning_plot()
  num_components_plot_alternate()
  type_components_plot()
  num_system_components_plot()
  func_plot()