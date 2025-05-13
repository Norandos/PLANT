import os
import subprocess
import serial
import mysql.connector
from mysql.connector import Error
import threading
import pandas as pd
import shutil
from zipfile import BadZipFile
from dotenv import load_dotenv

excel_lock = threading.Lock()

db_path = "/home/plant/OneDrive/Database_Copy/Database_sensors.xlsx"
temp_path = "/home/plant/Desktop/Database_sensors.xlsx"
load_dotenv("/home/plant/.env")


def insert_sensor_data(cursor, temperature, ec, ph, do, level):
    try:
        insert_query = """INSERT INTO Sensors (SystemID, Level, SensorType, Reading, Time) 
                          VALUES (%s, %s, %s, %s, NOW()), 
                                 (%s, %s, %s, %s, NOW()), 
                                 (%s, %s, %s, %s, NOW()), 
                                 (%s, %s, %s, %s, NOW())"""
        data = (1, level, 'Temperature', temperature,
                1, level, 'Conductivity', ec,
                1, level, 'pH', ph,
                1, level, 'Oxygen', do)
        cursor.execute(insert_query, data)
        print(f"Record inserted successfully into Sensors table for level {level}")

    except Error as e:
        print(f"Error while inserting data: {e}")

def read_serial_data(port, level):
    try:
        connection = mysql.connector.connect(
            host=os.getenv("DB_HOST"),
            database=os.getenv("DB_NAME"),
            user=os.getenv("DB_USER"),
            password=os.getenv("DB_PASS")
        )

        if connection.is_connected():
            cursor = connection.cursor()
            ser = serial.Serial(port, 9600, timeout=1)
            ser.reset_input_buffer()
            ser.dtr = False

            while True:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8').rstrip()
                    print(f"Data from {port}: {line}")
                    if line.count(",") > 2:
                        try:
                            temperature, ec, ph, do = map(float, line.split(','))
                            insert_sensor_data(cursor, temperature, ec, ph, do, level)
                            connection.commit()

                            with excel_lock:
                                update_excel_copy(temperature, ec, ph, do, level)

                        except ValueError as e:
                            print(f"Error parsing data from {port}: {e}")

    except Error as e:
        print(f"Error while connecting to MySQL: {e}")

    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()
            print(f"MySQL connection for {port} is closed")

def update_excel_copy(temp, ec, ph, do, level):
    try:
        columns = ["Time", "Temperature", "EC", "pH", "DO", "Level"]
        data = {
            "Time": [pd.Timestamp.now()],
            "Temperature": [temp],
            "EC": [ec],
            "pH": [ph],
            "DO": [do],
            "Level": [level]
        }

        dataframe = pd.DataFrame(data, columns=columns)

        if not os.path.exists(db_path):
            dataframe.to_excel(temp_path, index=False)
        else:
            existing_data = pd.read_excel(db_path)
            new_df = pd.concat([existing_data, dataframe], ignore_index=True)
            new_df.to_excel(temp_path, index=False)

        shutil.move(temp_path, db_path)
        print("Safely wrote to Excel file.")

        if subprocess.call(['pgrep', '-x', 'onedrive']) != 0:
            subprocess.run(['onedrive', '--synchronize'], check=True)
            print("sync success - " + str(pd.Timestamp.now()))
        else:
            print("OneDrive client is already running, skipping sync.")

    except Error as e:
        print(f"Error while inserting data to Excel: {e}")
    except BadZipFile:
        print("The Excel file is corrupted. Recreating it.")
        dataframe.to_excel(temp_path, index=False)
        shutil.move(temp_path, db_path)


if __name__ == '__main__':
    # Define your ports and corresponding levels
    ports_levels = [
        ('/dev/ttyUSB0', 3),
        ('/dev/ttyUSB1', 1)
    ]

    threads = []
    for port, level in ports_levels:
        thread = threading.Thread(target=read_serial_data, args=(port, level))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()
