#!/usr/bin/env python3
import serial
import mysql.connector
from mysql.connector import Error

def insert_sensor_data(temperature, ec, ph, do):
    try:
        connection = mysql.connector.connect(
            host='localhost',
            database='PLANT',
            user='plant',
            password='Saxion_IWT'
        )

        if connection.is_connected():
            cursor = connection.cursor()
            insert_query = """INSERT INTO Sensors (SystemID, Level, SensorType, Reading, Time) 
                              VALUES (%s, %s, %s, %s, NOW()), 
                                     (%s, %s, %s, %s, NOW()), 
                                     (%s, %s, %s, %s, NOW()), 
                                     (%s, %s, %s, %s, NOW())"""
            data = (1, 3, 'Temperature', temperature,
                    1, 3, 'Conductivity', ec,
                    1, 3, 'pH', ph,
                    1, 3, 'Oxygen', do)
            cursor.execute(insert_query, data)
            connection.commit()
            print("Record inserted successfully into Sensors table")

    except Error as e:
        print(f"Error while connecting to MySQL: {e}")

    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()
            print("MySQL connection is closed")

if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)  # /dev/ttyUSB0 (for personal)
    ser.reset_input_buffer()

    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').rstrip()
            print(line)
            try:
                temperature, ec, ph, do = map(float, line.split(','))
                insert_sensor_data(temperature, ec, ph, do)
            except ValueError as e:
                print(f"Error parsing data: {e}")
