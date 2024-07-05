# PLANT
This Document contains information on how to operate the PLANT - Hydroponic System. How to access and develop the Raspberry more. In case you are looking at this document from the SSO drive. Please go to Github: https://github.com/Norandos/PLANT and clone this repository and continue working on it. Do not use the SSO drive for version control, as this is a risky method.

## Connecting to the Raspberry
The best way of accessing the Raspberry in its current state, is by using a VNC Viewer. This can be done through either an Ethernet Cable, or by setting up a mobile hotspot. If one uses an Ethernet cable to setup the connection, one has to enable Network sharing on the device the user wants to operate the Raspberry from, as demonstrated in the image below. ![controlpanel](https://hackmd.io/_uploads/HJtf1DrvA.png)

The other method is using a mobile hotspot. One has to then use the LED touchscreen interface of the Raspberry to select what network to connect to. One could also simply plug a (wireless) mouse and/or key board into the Raspberry, as there are ports available. 

### SSH
Once having setup a network connection one can remotely connect to the raspberry from their own device. Either through ssh or a VNC viewer. When using SSH once can simply go into the terminal and type: 
```
ssh plant@ip-adress 
```
with the password **Saxion_IWT** (it could be plant)

the ip-adress can be seen on the LED screen of the Raspberry, or when connected through an ethernet cable one can simply use:
```
ssh plant@raspberry 
```
Using an SSH connection is only really recommended for simple operational activities, to restart the servers for example, or change configurations. 

### VNC
When performing more complex operations, or for accessing other features of the Raspbian OS, it is recommended to use a VNC viewer. In this case RealVNC viewer was used. ![afbeelding](https://hackmd.io/_uploads/HJlSfvSPR.png)
With this tool one can remotely access the Raspberry's Desktop OS. Here too, one can use the IP of 'raspberry' when using an Ethernet connection.

## The Circuit
The code for the Arduino Nano's is uniform. They all operate with the same code. The sensors that are connected to the Arduinos have to be calibrated first. For this the documentation of the supplier is recommended. To execute the calibration, one has to send commands to the arduino through the serial monitor. The exact wiring can be found in the diagram below. In this diagram all the red cables are for supplying 5V, the black wires are neutral. And the yellow cables are the data cables, 3 going to analog ports, while 1 goes to a digital with a pull-up resistor of 4.7K. This diagram also shows the different sensors used. Refer to their documentation for guidance on setting up.

![exact layout](https://hackmd.io/_uploads/S15kYPSwA.png)

## The Arduino Code ([HydrophonicSystemSensorCode.ino](https://github.com/Norandos/PLANT/blob/main/HydrophonicSystemSensorCode/HydrophonicSystemSensorCode.ino)
This code is designed to measure and report pH, electrical conductivity (EC), temperature, and dissolved oxygen (DO) levels using various sensors. The program initializes and reads data from these sensors, performing necessary calibrations and compensation for accurate measurements. The results are printed in a CSV format to the serial monitor at regular intervals.

If one wants to change the code, one has to simply make changes to the code and then use an IDE (Like Arduino IDE) to upload hte code to an arduino. through the micro-USB cable. (So either with the Raspberry or through your own device)

#### Hardware and Libraries:
- **Libraries**:
  - `DFRobot_PH`: For pH measurement.
  - `EEPROM`: For storing calibration data.
  - `OneWire` and `DallasTemperature`: For temperature measurement using a DS18B20 sensor.
  - `DFRobot_EC`: For electrical conductivity measurement.
- **Pins**:
  - `PH_PIN (A1)`: pH sensor input.
  - `EC_PIN1 (A3)`: Electrical conductivity sensor input.
  - `OX_PIN (A4)`: Dissolved oxygen sensor input.
  - `ONE_WIRE_BUS (4)`: Temperature sensor data line.

#### Key Definitions:
- **Voltage Reference (VREF)**: 5000mV.
- **ADC Resolution (ADC_RES)**: 1024.
- **Calibration**:
  - Single-point calibration parameters: `CAL1_V (820mV)` and `CAL1_T (17℃)`.
  - Two-point calibration parameters: `CAL2_V (1260mV)` and `CAL2_T (31℃)`.
- **DO_Table**: Lookup table for dissolved oxygen concentration at different temperatures.

#### Workflow:
1. **Initialization**:
   - Serial communication starts at 9600 baud.
   - Sensors are initialized.

2. **Loop**:
   - Reads sensor values:
     - `getpH()`: Reads and compensates pH value.
     - `getEc()`: Reads and compensates EC value.
     - `getTempSensor()`: Reads temperature value.
     - `getOxygenSensor()`: Reads and compensates DO value.
   - Skips the first reading to avoid faulty data.
   - Prints the sensor data in CSV format to the serial monitor.
   - Waits for 30 minutes before the next reading (blocking delay).

#### Functions:
- **getEc()**: Measures EC, compensates with temperature.
- **getpH()**: Measures pH, compensates with temperature.
- **getTempSensor()**: Reads temperature using the Dallas temperature sensor.
- **getOxygenSensor()**: Measures dissolved oxygen concentration, converts ADC voltage to DO concentration.
- **readDO(voltage_mv, temperature_c)**: Calculates DO concentration based on voltage and temperature, supporting two-point calibration.

## Python Code (DataParsing.py)
> The code is run under PLANT/home/DataParsing.py
> 
> There is a service running named DataParsing that starts the DataParsing.py code upon boot up. If one needs to debug the python code, one has to open the code in PLANT/home/DataParsing.py with an editor and execute it in a terminal, this will give more information on which data gets sent to the database.

![image](https://hackmd.io/_uploads/SkrORvBPR.png)


This Python script reads sensor data from multiple serial ports, parses it, and inserts it into a MySQL database. The data includes temperature, electrical conductivity (EC), pH, and dissolved oxygen (DO) levels. The script uses multithreading to handle multiple ports simultaneously.

#### Libraries and Modules:
- `serial`: For reading data from serial ports.
- `mysql.connector`: For connecting to and interacting with a MySQL database.
- `threading`: For running multiple threads.
- `time`: For handling timing operations.

#### Functions:
1. **insert_sensor_data(cursor, temperature, ec, ph, do, level)**:
   - Inserts sensor data into the MySQL database.
   - Parameters:
     - `cursor`: MySQL cursor object.
     - `temperature`: Temperature reading.
     - `ec`: Electrical conductivity reading.
     - `ph`: pH reading.
     - `do`: Dissolved oxygen reading.
     - `level`: Level identifier for the data.
   - Executes an SQL insert query to add the data to the `Sensors` table.

2. **read_serial_data(port, level)**:
   - Reads data from a specified serial port and inserts it into the MySQL database.
   - Parameters:
     - `port`: Serial port to read data from.
     - `level`: Level identifier for the data.
   - Connects to the MySQL database.
   - Continuously reads lines from the serial port, parses them, and inserts the parsed data into the database.
   - Handles connection and parsing errors, ensuring the database connection is closed properly.

#### Workflow:
1. **Initialization**:
   - Connect to the MySQL database using the specified credentials.
   - Initialize the serial port with a baud rate of 9600 and a timeout of 1 second.
   - Reset the input buffer to clear any existing data.

2. **Data Reading and Insertion**:
   - Continuously check for incoming data on the serial port.
   - Read a line of data, decode it, and strip any trailing characters.
   - Parse the data into temperature, EC, pH, and DO values.
   - Insert the parsed data into the MySQL database using the `insert_sensor_data` function.
   - Commit the transaction to save the data in the database.
   - Handle any errors that occur during data parsing or database insertion.

3. **Multithreading**:
   - Define the ports and corresponding levels to be monitored.
   - Create and start a thread for each port, running the `read_serial_data` function.
   - Wait for all threads to complete before exiting the script.

> To Determine to which ports the Arduinos are connected, one can run the command: `ls /dev/tty*` to see which devices are connected to the Raspberry. By plugging the arduinos in one by one while using that command, one can determine which is which. It is also possible to define port levels by the serial number of the Arduino's. Guides for this can easily be found online.

#### Example Usage:
```python
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
```

## Database
The easiest way of accessing the database is by using phpmyadmin. Phpmyadmin is a creat tool for administration of a database with an easy to use interface. It can be accessed by opening a browser and going to the url:

```
localhost/phpmyadmin (when on the raspberry)
ip-adress/phpmyadmin (when connecting remotely)
```
### Database Structure for Environmental Monitoring System

The database structure for the environmental monitoring system includes three tables: `Systems`, `SensorTypes`, and `Sensors`. Below is a summary of each table and its columns based on the provided image.

#### Tables and Columns

1. **Systems**
   - **SystemID**: Integer (Primary Key)
   - **SystemName**: Varchar(50)

2. **SensorTypes**
   - **SensorTypeID**: Integer (Primary Key)
   - **SensorName**: Varchar(50)

3. **Sensors**
   - **SensorID**: Integer (Primary Key, Auto Increment)
   - **SystemID**: Integer (Foreign Key referencing `Systems.SystemID`)
   - **SensorTypeID**: Integer (Foreign Key referencing `SensorTypes.SensorTypeID`)
   - **Reading**: Float
   - **Time**: Timestamp

#### Relationships

- Each **sensor** record is linked to one **system** and one **sensor type**.
- The **SystemID** in the `Sensors` table references the `SystemID` in the `Systems` table.
- The **SensorTypeID** in the `Sensors` table references the `SensorTypeID` in the `SensorTypes` table.

#### Example SQL Queries

**Creating the Tables:**

```sql
CREATE TABLE Systems (
    SystemID INT PRIMARY KEY,
    SystemName VARCHAR(50)
);

CREATE TABLE SensorTypes (
    SensorTypeID INT PRIMARY KEY,
    SensorName VARCHAR(50)
);

CREATE TABLE Sensors (
    SensorID INT PRIMARY KEY AUTO_INCREMENT,
    SystemID INT,
    SensorTypeID INT,
    Reading FLOAT,
    Time TIMESTAMP,
    FOREIGN KEY (SystemID) REFERENCES Systems(SystemID),
    FOREIGN KEY (SensorTypeID) REFERENCES SensorTypes(SensorTypeID)
);
```

**Example Inserting Data into the Tables:**

```sql
-- Insert into Systems
INSERT INTO Systems (SystemID, SystemName) VALUES (1, 'System A');

-- Insert into SensorTypes
INSERT INTO SensorTypes (SensorTypeID, SensorName) VALUES (1, 'Temperature');
INSERT INTO SensorTypes (SensorTypeID, SensorName) VALUES (2, 'Conductivity');
INSERT INTO SensorTypes (SensorTypeID, SensorName) VALUES (3, 'pH');
INSERT INTO SensorTypes (SensorTypeID, SensorName) VALUES (4, 'Oxygen');

-- Insert into Sensors
INSERT INTO Sensors (SystemID, SensorTypeID, Reading, Time) VALUES (1, 1, 25.3, NOW());
INSERT INTO Sensors (SystemID, SensorTypeID, Reading, Time) VALUES (1, 2, 1.2, NOW());
INSERT INTO Sensors (SystemID, SensorTypeID, Reading, Time) VALUES (1, 3, 7.1, NOW());
INSERT INTO Sensors (SystemID, SensorTypeID, Reading, Time) VALUES (1, 4, 8.6, NOW());
```

This structure ensures a normalized and efficient database design, allowing for the proper storage and retrieval of sensor data related to various systems and sensor types.
![image](https://hackmd.io/_uploads/Bkym8wBPR.png)


## Dashboard
> To configure grafana, go to configuration file `/etc/grafana/grafana.ini`

The Dashboard can be accessed in a similar way as with the database as grafana is hosted on the port 3000. 

```
localhost:3000 (when on the raspberry)
ip-adress:3000 (when connecting remotely)
```

When initially accessing the dashboard, one will connect through anonymous access. This is public so that anyone connected can look at the data, but not change the dashboard. To actually change the dashboard, one needs to login as the administrator. In the top right of the Dashboard one can login as the admin with the following credentials:

```
name: admin
password: plant
```

Once logged in as admin, one can add more visualizations to the dashboard by preforming SQL queries. In case you are new to Grafana, I recommend following a beginners guide to Grafana before starting to change the settings. A plugin has been installed to enable the use of interactive buttons. These could then send commands to the Raspberry through an API. This has not been implemented yet though.
