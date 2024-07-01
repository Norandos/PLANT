#!/usr/bin/env python3
import serial
import mysql.connector
from mysql.connector import Error
import threading
import time

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
            host='localhost',
            database='PLANT',
            user='plant',
            password='Saxion_IWT'
        )

        if connection.is_connected():
            cursor = connection.cursor()
            ser = serial.Serial(port, 9600, timeout=1)
            ser.reset_input_buffer()

            while True:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8').rstrip()
                    print(f"Data from {port}: {line}")
                    try:
                        temperature, ec, ph, do = map(float, line.split(','))
                        insert_sensor_data(cursor, temperature, ec, ph, do, level)
                        connection.commit()
                    except ValueError as e:
                        print(f"Error parsing data from {port}: {e}")

    except Error as e:
        print(f"Error while connecting to MySQL: {e}")

    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()
            print(f"MySQL connection for {port} is closed")

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
