#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>
#include <stdexcept>

enum class VehicleType { CAR, BIKE, TRUCK };

struct Slot {
    int id;
    bool occupied;
    std::string vehicleNumber;
    VehicleType type;
    long entryTimeEpoch;  
};

struct ParkResult {
    bool success;
    std::string message;
    int slotId;
};

struct ReleaseResult {
    bool success;
    std::string message;
    double amount;
    long durationMinutes;
};

class ParkingLot {
public:
    explicit ParkingLot(const std::string& dbPath, int totalSlots);
    ~ParkingLot();

    // Core operations
    ParkResult parkVehicle(const std::string& vehicleNumber, VehicleType type);
    ReleaseResult releaseVehicle(const std::string& vehicleNumber);
    std::vector<Slot> getOccupiedSlots();
    std::vector<Slot> getAllSlots();
    int getFreeSlotCount();

    std::string getHistoryJson();

    static std::string typeToString(VehicleType t);
    static VehicleType stringToType(const std::string& s);

private:
    sqlite3* db;
    int totalSlots;

    void initSchema();
    double calculateFee(VehicleType type, long durationMinutes);
    int findFreeSlotId();
};
