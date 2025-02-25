import csv
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

sorted_values = [
"auth_delay",
"basebackup_to_shell",
"auto_explain",
"pg_log_userqueries",
"dont_drop_db",
"plv8",
"sslinfo",
"adminpack",
"basic_archive",
"intagg",
"pgjwt",
"uuid-ossp",
"old_snapshot",
"pg_buffercache",
"pg_prewarm",
"pg_proctab",
"pgrowlocks",
"pgtap",
"tcn",
"wal2json",
"xml2",
"passwordcheck",
"pg_partman",
"orafce",
"pgfincore",
"test_decoding",
"dblink",
"dict_int",
"fuzzystrmatch",
"lo",
"dict_xsyn",
"hypopg",
"pg_freespacemap",
"pg_variables",
"pg_visibility",
"pgcrypto",
"pgstattuple",
"tablefunc",
"tsm_system_time",
"amcheck",
"pageinspect",
"pg_surgery",
"pg_wait_sampling",
"tsm_system_rows",
"unaccent",
"jsonb_plpython3u",
"logerrors",
"pg_walinspect",
"bool_plperl",
"jsonb_plperl",
"pgtt",
"shared_ispell",
"file_fdw",
"imcs",
"pg_qualstats",
"hstore_plpython3u",
"ltree_plpython3u",
"pg_ivm",
"plprofiler",
"citext",
"postgres_fdw",
"pg_trgm",
"hll",
"ip4r",
"vops",
"hstore_plperl",
"pg_repack",
"pgaudit",
"hstore",
"intarray",
"ltree",
"cube",
"bloom",
"btree_gist",
"pg_stat_statements",
"prefix",
"seg",
"btree_gin",
"pg_bigm",
"pg_hint_plan",
"vector",
"isn",
"lsm3",
"pg_show_plans",
"citus",
"pg_tle",
"pgsentinel",
"pg_cron",
"pg_stat_kcache",
"pg_query_rewrite",
"tds_fdw",
"earthdistance",
"pg_stat_monitor",
"timescaledb",
"pg_queryid",
"pgextwlist",
]

abc_order_list = sorted_values.copy()
abc_order_list.sort()

# Removing pgextwtlist
sorted_values.pop()
num_extensions = len(sorted_values)

compatibility_file = open("csvs/compatibility_results.csv", "r")
compatibility_dict = {}

csv_reader = csv.reader(compatibility_file, delimiter=',')
line_count = 0

yes_count = 0
no_count = 0
for row in csv_reader:
    if line_count > 0:
        #print(row)
        second_extension = row[0]
        for i in range(1, len(row)):
            first_extension = abc_order_list[i-1]
            #print(first_extension + ", " + second_extension)
            if first_extension != second_extension:
                val = False
                if row[i] == "yes":
                    val = True
                    yes_count += 1
                elif row[i] == "no":
                    no_count += 1

                #print(val)
                compatibility_dict[(first_extension, second_extension)] = val

    line_count += 1

index_type_df = pd.read_csv('../../analysis-scripts/csvs/index_type_results.csv')
for _, row in index_type_df.iterrows():
    type_extn = row['Type']
    index_extn = row['Index']
    compatible_val = row['Compatible']

    if compatible_val == False:
      compatibility_dict[(type_extn, index_extn)] = False
      compatibility_dict[(index_extn, type_extn)] = False


image = np.zeros((num_extensions, num_extensions, 3), dtype=np.uint8) 

#BOUND_IDX = 51
for first_idx in range(num_extensions):
    for second_idx in range(num_extensions):
        first_extn = sorted_values[first_idx]
        second_extn = sorted_values[second_idx]
        if first_extn == second_extn:
            image[first_idx, second_idx] = (255,244,204)
        else:
            val = compatibility_dict[(first_extn, second_extn)]
            if val:
                image[first_idx, second_idx] = (184,228,204)
            else:
                image[first_idx, second_idx] = (232,0,11)
                #if first_idx <= BOUND_IDX and second_idx <= BOUND_IDX:
                    #print((first_idx, second_idx))

def is_cond(extn):
    return extn == "pg_repack" or extn == "prefix"

count = 0
small_image = np.zeros((20, 20, 3), dtype=np.uint8)
for first_idx in range(20):
    for second_idx in range(20):
        first_extn = sorted_values[first_idx + num_extensions - 20]
        second_extn = sorted_values[second_idx + num_extensions - 20]
        if first_extn == second_extn:
            small_image[first_idx, second_idx] = (255,244,204)
        else:
            val = compatibility_dict[(first_extn, second_extn)]
            if val:
                small_image[first_idx, second_idx] = (184,228,204)
            else:
                print(second_extn)
                small_image[first_idx, second_idx] = (232,0,11)

print(count)

def small_graph():
    _, ax = plt.subplots(figsize=(8, 8))
    ax.imshow(small_image, interpolation='nearest')

    # Hide grid lines
    ax.grid(False)

    font_size = 10
    print(sorted_values[-20:])
    ax.set_yticks([0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19])
    ax.set_yticklabels(sorted_values[-20:], fontsize=font_size)
    ax.set_xticks([0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19])
    ax.set_xticklabels(sorted_values[-20:], rotation=10, fontsize=font_size)
    ax.tick_params(axis='both', which='both', length=0)  # Hides both major and minor ticks
    ax.spines['top'].set_visible(False)  # Hide the top spine
    ax.spines['right'].set_visible(False)  # Hide the right spine
    ax.spines['bottom'].set_visible(False)  # Hide the bottom spine
    ax.spines['left'].set_visible(False)  # Hide the left spine

    plt.setp(ax.get_xticklabels(), rotation=30, horizontalalignment='right')

    # Save or show the plot
    plt.savefig("small_test.pdf", bbox_inches='tight', pad_inches=0)
    plt.show()

def big_graph():
    _, ax = plt.subplots(figsize=(8, 8))
    ax.imshow(image, interpolation='nearest')

    # Hide grid lines
    ax.grid(False)
    ax.set_axis_off()

    # Save or show the plot
    plt.savefig("big_test.pdf", bbox_inches='tight', pad_inches=0)
    plt.show()

small_graph()
big_graph()