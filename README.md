# BlueNovaBatteryMonitor
This small utility opens a websocket connection to BlueNova LiFeYPO4 batteries which supports a web interface on port 9000. It parses the console output stream and logs the results in a PostgeSQL database. From there it can be graphed/monitored with tools like Grafana

## Compiling and installation
The code was tested on FreeBSD  but should compile on most Unix-like systems.

 1. Ensure that [libwebsockets](https://libwebsockets.org/) is installed
 2. Ensure the PostgreSQL client and development packages are installed (package names vary by system type)
 3. Create the database table battery_pack, described below
 4. Compile monitor code: `gcc -g -l pgeasy -l pq -lpthread -lwebsockets -L/usr/local/lib -I/usr/local/include -o bluenovalistener BlueNovaListenerWS.c`
 5. Copy the `bluenovalistener` executable to /usr/local/bin
 6. Add an rc.d entry to auto-start the listener on start-up
 7. Multiple instances of `bluenovalistener` can be run, all writing to the same database table, to monitor multiple batteries.

## Database table creation
The following statement will create the required database table:

    create table battery_pack
    (
     INSERT_TIME_UTC   TIMESTAMP,
     MCU_ID            BIGINT,
     N_UNITS           SMALLINT,
     UNIT_RANK         SMALLINT,
     CELL_COUNT        SMALLINT,
     CELL_01_V         REAL,
     CELL_02_V         REAL,
     CELL_03_V         REAL,
     CELL_04_V         REAL,
     CELL_05_V         REAL,
     CELL_06_V         REAL,
     CELL_07_V         REAL,
     CELL_08_V         REAL,
     CELL_09_V         REAL,
     CELL_10_V         REAL,
     CELL_11_V         REAL,
     CELL_12_V         REAL,
     CELL_13_V         REAL,
     CELL_14_V         REAL,
     CELL_15_V         REAL,
     CELL_16_V         REAL,
     CELL_01_T         SMALLINT,
     CELL_02_T         SMALLINT,
     CELL_03_T         SMALLINT,
     CELL_04_T         SMALLINT,
     CELL_05_T         SMALLINT,
     CELL_06_T         SMALLINT,
     CELL_07_T         SMALLINT,
     CELL_08_T         SMALLINT,
     CELL_09_T         SMALLINT,
     CELL_10_T         SMALLINT,
     CELL_11_T         SMALLINT,
     CELL_12_T         SMALLINT,
     CELL_13_T         SMALLINT,
     CELL_14_T         SMALLINT,
     CELL_15_T         SMALLINT,
     CELL_16_T         SMALLINT,
     V_PACK            REAL,
     I_PACK            REAL,
     TEMP              SMALLINT,
     V_CELL_MIN        REAL,
     V_CELL_MAX        REAL,
     SOC               SMALLINT,
     HEALTH            REAL,
     ERRORS            VARCHAR(32),
     SYSTEM_STATE      VARCHAR(32),
     BREAKER           VARCHAR(32),
     CHARGE_STATE      VARCHAR(16),
     ENERGY_IN         REAL,
     ENERGY_OUT        REAL,
     ADC_VP            REAL,
     ADC_0             REAL,
     ADC_1             REAL,
     ADC_2             REAL,
     ADC_3             REAL,
     ADC_4             REAL,
     BAD_CRC           SMALLINT,
     PARSE_FAIL        SMALLINT,
     BAD_VCELL         SMALLINT
    );
    create index battery_pack_time on battery_pack(insert_time_utc);

## Command line parameters
The command-line parameters are as follows:

    bluenovalistener [-daemon] [-b battery-ip] [-d postgresql-DB-name] [-u postgresql-user-name] [-i interval_secs]
                     [-l logfile-name] [-n maximum-keep-logfiles] [-m max-logfile-size] [-f flush-log-flag] [-r]

`-daemon` runs as daemon process on unix systems
`battery-ip` hostname or IP address assigned to the battery - default `10.0.0.2`
`postgresql-DB-name` name of PostgreSQL database to use - default `homeauto`
`postgresql-user-name`PostgreSQL username to use - default `homeauto`. PostgreSQL authentication must be configured to not ask a password for this user/process
`interval_secs` How often to write the latest battery statistics to the database - default every 10 seconds
`logfile-name` Name to use for log files - default logs to the console
`maximum-keep-logfiles`Number of old log files to keep when rotating - default 5
`max-logfile-size` Maximum size per log file - default 50MB
`flush-log-flag` Immediately flush log file output after each line - value `0` (don't flush) or `1` (flush). Default is `0`
`-r` Log raw console data from battery as it is received

# Notes:

## Wired Ethernet
The BlueNova NG batteries contain a Raspberry Pi Zero W which runs the BlueNova monitoring application and provides WiFi connectivity. The USB socket on the front panel is directly connected to the Pi - this means you can plug in USB devices like you would in a normal Pi.

One possibility is a USB Ethernet adapter - I prefer my devices to be on a fixed network. An adapter based on the AX88772B chipset works great.

**CAVEAT**: The battery assigns a static IP of 10.0.0.2/24 to the Ethernet adapter - if you need a different address or have multiple battery units you will have to open the battery and update the Pi's SD card.

Rough procedure:

 **WARNING** The battery contains exposed bus bars inside and the cover is stainless steel - be very careful as a short circuit will be extremely unpleasant. Do not attempt this unless you really know what you are doing.
 

 1. (optional) Discharge battery to almost empty
 2. Turn off battery switch and any MPPTs or inverters connected to the battery - nothing external should power the battery. The OLED display on the battery should now to totally off
 3. Remove battery cover; carefully remove SD card from the Pi
 4. Mount the SD card on another Linux system
 5. Edit the `/etc/network/interfaces` file and replace 


        iface eth0 inet static
            address 10.0.0.2/24
            dns-nameservers 8.8.8.8 4.4.4.4

     with

        iface eth0 inet dhcp
 6. (optional) Add your own ssh public key to `/home/pi/.ssh/authorized_keys`  to enable you to ssh into the Pi.
 7. Unmount SD card and replace in the Pi
 8. (optional) Turn on battery alone and verify that network connectivity and ssh is working as expected.
 9. Close battery cover, turn on batteries, inverters and MPPTs.

## InfluxDB
The Pi in the BlueNova batteries runs InfluxDB (v1.0.2 on mine, with an open admin web interface on port 8083). This process is extremely heavy for the Pi leaving no idle time and causing reboots on my one battery.

I've gone and stopped/disabled the InfluxDB daemon with:

    sudo systemctl stop influxd.service
    sudo systemctl disable influxd.service
    
The result is a lot less CPU load and disk writes to the SD card (longer life)

Stopping InfluxDB does not seem to affect monitoring at all - but may affect data upload to brm.bluenova.co.za
