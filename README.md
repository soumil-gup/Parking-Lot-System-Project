# Parking Lot Management System

A full-stack parking lot management system with a **C++ REST API backend**
(Crow framework), **SQLite persistence**, and a **live web dashboard**.

## Features

- Park a vehicle → auto-assigns the next free slot
- Release a vehicle → auto-calculates the parking fee based on duration and vehicle type
- Live dashboard showing free/occupied slots, updated every 5 seconds
- Full parking history log (active + completed sessions)
- Rejects double-parking (same vehicle number twice) and parking when the lot is full
- Data survives restarts (SQLite file, not in-memory)

## Tech Stack

| Layer      | Technology |
|------------|-----------|
| Backend    | C++17, [Crow](https://github.com/CrowCpp/Crow) (micro web framework) |
| Database   | SQLite3 |
| Frontend   | HTML / CSS / vanilla JS (fetches the REST API) |
| Build      | CMake or plain g++ |

## Project Structure

```
parking-lot-system/
├── include/crow/        # Crow framework headers (header-only)
├── include/crow.h
├── src/
│   ├── ParkingLot.hpp    # Core OOP classes (Slot, ParkingLot) + SQLite logic
│   ├── ParkingLot.cpp
│   └── main.cpp          # REST API routes (Crow)
├── frontend/
│   └── index.html        # Dashboard UI (HTML/CSS/JS)
├── CMakeLists.txt
└── README.md
```

## Setup & Build

### Prerequisites (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libsqlite3-dev libasio-dev libboost-dev
```

Crow needs either standalone `asio` or `boost::asio` under the hood —
`libasio-dev` is the lighter option and is what this project was built with.

### Build with CMake (recommended)

```bash
mkdir build && cd build
cmake ..
make
./parking_server
```

### Build with plain g++ (no CMake needed)

```bash
g++ -std=c++17 -O2 -Iinclude src/main.cpp src/ParkingLot.cpp -lsqlite3 -lpthread -o parking_server
./parking_server
```

Then open your browser at:

```
http://localhost:18080
```
# Parking Lot Management System

A full-stack parking lot management system with a **C++ REST API backend**
(Crow framework), **SQLite persistence**, and a **live web dashboard**.

## Features

- Park a vehicle → auto-assigns the next free slot
- Release a vehicle → auto-calculates the parking fee based on duration and vehicle type
- Live dashboard showing free/occupied slots, updated every 5 seconds
- Full parking history log (active + completed sessions)
- Rejects double-parking (same vehicle number twice) and parking when the lot is full
- Data survives restarts (SQLite file, not in-memory)

## Tech Stack

| Layer      | Technology |
|------------|-----------|
| Backend    | C++17, [Crow](https://github.com/CrowCpp/Crow) (micro web framework) |
| Database   | SQLite3 |
| Frontend   | HTML / CSS / vanilla JS (fetches the REST API) |
| Build      | CMake or plain g++ |

## Project Structure

```
parking-lot-system/
├── include/crow/        # Crow framework headers (header-only)
├── include/crow.h
├── src/
│   ├── ParkingLot.hpp    # Core OOP classes (Slot, ParkingLot) + SQLite logic
│   ├── ParkingLot.cpp
│   └── main.cpp          # REST API routes (Crow)
├── frontend/
│   └── index.html        # Dashboard UI (HTML/CSS/JS)
├── CMakeLists.txt
└── README.md
```

## Setup & Build

### Prerequisites (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libsqlite3-dev libasio-dev libboost-dev
```

Crow needs either standalone `asio` or `boost::asio` under the hood —
`libasio-dev` is the lighter option and is what this project was built with.

### Build with CMake (recommended)

```bash
mkdir build && cd build
cmake ..
make
./parking_server
```

### Build with plain g++ (no CMake needed)

```bash
g++ -std=c++17 -O2 -Iinclude src/main.cpp src/ParkingLot.cpp -lsqlite3 -lpthread -o parking_server
./parking_server
```

Then open your browser at:

```
http://localhost:18080
```

A `parking.db` SQLite file is created automatically in the folder the
server runs from — it holds all slots and history, so data survives restarts.

## REST API Reference

| Method | Endpoint         | Body                                              | Description                         |
|--------|------------------|----------------------------------------------------|--------------------------------------|
| GET    | `/api/status`    | –                                                  | Get all slots + free/total counts   |
| POST   | `/api/park`      | `{"vehicle_number": "MH20AB1234", "type": "CAR"}`  | Park a vehicle                      |
| POST   | `/api/release`   | `{"vehicle_number": "MH20AB1234"}`                 | Release a vehicle, get the bill     |
| GET    | `/api/history`   | –                                                  | Last 100 parking records            |

`type` can be `CAR`, `BIKE`, or `TRUCK` (defaults to `CAR` if omitted).

### Example with curl

```bash
curl -X POST http://localhost:18080/api/park \
  -H "Content-Type: application/json" \
  -d '{"vehicle_number":"MH20AB1234","type":"CAR"}'

curl -X POST http://localhost:18080/api/release \
  -H "Content-Type: application/json" \
  -d '{"vehicle_number":"MH20AB1234"}'
```
