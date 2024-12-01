import sqlite3
import csv
from datetime import datetime, timedelta
import json
import os
import subprocess
import random
import threading
import queue

SINGLE = False

# File paths (globals)
current_working_dir = os.getcwd()
ext_work_dir = "sqliteextworkdir"
extn_info_dir = "extn_info"
extn_code_dir = "extn_code"
now = datetime.now()
date_time = now.strftime("%m-%d-%Y_%H:%M")
testing_output_dir = "testing-output-" + date_time

# Load extension database
extn_files = os.listdir(current_working_dir + "/" + extn_info_dir)
extn_db = {}
for file in extn_files:
  extn_info_file = open(current_working_dir + "/" + extn_info_dir + "/" + file, "r")
  extn_info_json = json.load(extn_info_file)
  key = os.path.splitext(file)[0]
  extn_db[key] = extn_info_json

# Helpers
def init():
  subprocess.run("mkdir " + ext_work_dir, shell=True, cwd=current_working_dir)
  subprocess.run("mkdir " + testing_output_dir, shell=True, cwd=current_working_dir)

def cleanup():
  subprocess.run("rm -rf " + ext_work_dir, shell=True, cwd=current_working_dir)

def generate_product_data(num_rows=50):
    """
    Generate sample product data
    """
    categories = [
        'Electronics', 'Clothing', 'Books', 
        'Home & Kitchen', 'Sports', 'Toys'
    ]
    
    products = []
    for i in range(num_rows):
        # Generate realistic product data
        product = (
            i + 1,  # id
            f'Product {i+1}',  # name
            random.choice(categories),  # category
            round(random.uniform(10.99, 599.99), 2),  # price
            random.randint(0, 500),  # stock quantity
            datetime.now() - timedelta(days=random.randint(0, 365))  # last updated
        )
        products.append(product)
    
    return products

def insert_products(cursor, products):
    """
    Insert products into the database
    """
    cursor.executemany('''
    INSERT INTO products 
    (id, name, category, price, stock_quantity, last_updated) 
    VALUES (?, ?, ?, ?, ?, ?)
    ''', products)

def run_queries(cursor):
    """
    Demonstrate various SQL queries
    """
    #print("\n--- Query 1: Products in Electronics category ---")
    cursor.execute('''
    SELECT * FROM products 
    WHERE category = 'Electronics' 
    AND price > 100 
    ORDER BY price DESC 
    LIMIT 5
    ''')
    electronics = cursor.fetchall()
    #for product in electronics:
    #    print(f"ID: {product[0]}, Name: {product[1]}, Price: ${product[3]:.2f}")

    #print("\n--- Query 2: Low stock products ---")
    cursor.execute('''
    SELECT name, category, stock_quantity 
    FROM products 
    WHERE stock_quantity < 50 
    ORDER BY stock_quantity
    ''')
    low_stock = cursor.fetchall()
    #for product in low_stock:
    #    print(f"Name: {product[0]}, Category: {product[1]}, Stock: {product[2]}")

    #print("\n--- Query 3: Average price by category ---")
    cursor.execute('''
    SELECT category, 
           ROUND(AVG(price), 2) as avg_price, 
           COUNT(*) as product_count
    FROM products 
    GROUP BY category 
    ORDER BY avg_price DESC
    ''')
    category_stats = cursor.fetchall()
    #for stat in category_stats:
    #    print(f"Category: {stat[0]}, Avg Price: ${stat[1]}, Products: {stat[2]}")
    return category_stats


def test_pairwise(extn1, extn2):
  print("Testing extensions " + extn1 + " and " + extn2)
  compat_val = True
  timeout = 30
  results_queue = queue.Queue()

  def query_thread():
    try:
      print("Oops")
      con = sqlite3.connect(":memory:")
      con.enable_load_extension(True)
      e1_soname = current_working_dir + "/shared_objects/" + extn1 + ".so"
      e2_soname = current_working_dir + "/shared_objects/" + extn2 + ".so"
      print("Hello0")
      con.load_extension(e1_soname, entrypoint=entry_point_arg(extn1))
      print("Hello 0.5")
      con.load_extension(e2_soname, entrypoint=entry_point_arg(extn2))
      print("Hello1")
      cursor = con.cursor()
      print("Hello2")
      
      # Create products table
      cursor.execute('''
      CREATE TABLE products (
          id INTEGER PRIMARY KEY,
          name TEXT NOT NULL,
          category TEXT,
          price REAL,
          stock_quantity INTEGER,
          last_updated DATETIME
      )''')

      print("Hello3")
      products = generate_product_data()
      insert_products(cursor, products)
      print("Hello4")

      category_stats = run_queries(cursor)
      print("Hello5")
      results_queue.put(category_stats)

    except sqlite3.Error as e:
      print(f"SQLite error: {e}")
      results_queue.put(e)
    except Exception as e:
      results_queue.put(e)

    finally:
        # Close the connection
        if con:
            con.close()

  # Create and start the query thread
  thread = threading.Thread(target=query_thread)
  #thread.daemon = True  
  thread.start()
  
  # Wait for the specified timeout
  thread.join(timeout)

  if thread.is_alive():
     compat_val = False

  result = results_queue.get()
  if isinstance(result, Exception):
    compat_val = False

  return compat_val

def entry_point_arg(extn):
   extn_entry = extn_db[extn]
   if "entry_point" in extn_entry:
      return extn_entry["entry_point"]
   
   return None

def test_single_load(extn):
  print("Testing extension load " + extn)
  try:
    con = sqlite3.connect(":memory:")
    con.enable_load_extension(True)
    e_soname = current_working_dir + "/shared_objects/" + extn + ".so"
    print(e_soname)
    con.load_extension(e_soname, entrypoint=entry_point_arg(extn))
  except sqlite3.Error as e:
        print(f"SQLite error: {e}")
  finally:
      # Close the connection
      if con:
          con.close()

  return True

if __name__ == '__main__':
  init()

  shared_obj_files = os.listdir(current_working_dir + "/shared_objects")
  extn_list = list(extn_db.keys())
  extn_list.sort()
  extn_pairs = []
  
  # Output file
  compat_file = open("compatibility.csv", "w")
  compat_csv = csv.writer(compat_file)
  compat_csv.writerow(["Extension 1", "Extension 2", "Result"])

  if SINGLE:
    for e in extn_list:
      e1_soname = e + ".so"
      if e1_soname not in shared_obj_files:
        continue
      test_single_load(e)
  else:
    for e1 in extn_list:
      e1_soname = e1 + ".so"
      if e1_soname not in shared_obj_files:
        continue

      for e2 in extn_list:
        e2_soname = e2 + ".so"
        if e2_soname not in shared_obj_files:
          continue

        # Hardcoding vfsstat hanging
        #if e1 == "vfsstat":
        #   compat_csv.writerow([e1, e2, "False"])
        #   continue

        compat = test_pairwise(e1, e2)
        compat_csv.writerow([e1, e2, str(compat)])

  cleanup()