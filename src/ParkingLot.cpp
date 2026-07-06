#include "ParkingLot.hpp"
#include <sstream>
#include <ctime>
#include <iostream>

ParkingLot::ParkingLot(const std::string& dbPath, int totalSlots)
    : db(nullptr), totalSlots(totalSlots) {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Cannot open database: " + std::string(sqlite3_errmsg(db)));
    }
    initSchema();
}

ParkingLot::~ParkingLot() {
    if (db) sqlite3_close(db);
}

void ParkingLot::initSchema() {
    const char* slotsTable =
        "CREATE TABLE IF NOT EXISTS slots ("
        "id INTEGER PRIMARY KEY, "
        "occupied INTEGER NOT NULL DEFAULT 0, "
        "vehicle_number TEXT DEFAULT '', "
        "type TEXT DEFAULT '', "
        "entry_time INTEGER DEFAULT 0"
        ");";

    const char* historyTable =
        "CREATE TABLE IF NOT EXISTS history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "vehicle_number TEXT NOT NULL, "
        "slot_id INTEGER NOT NULL, "
        "type TEXT NOT NULL, "
        "entry_time INTEGER NOT NULL, "
        "exit_time INTEGER, "
        "amount REAL"
        ");";

    char* errMsg = nullptr;
    sqlite3_exec(db, slotsTable, nullptr, nullptr, &errMsg);
    sqlite3_exec(db, historyTable, nullptr, nullptr, &errMsg);

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM slots;", -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (count == 0) {
        sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg);
        for (int i = 1; i <= totalSlots; ++i) {
            std::ostringstream q;
            q << "INSERT INTO slots (id, occupied) VALUES (" << i << ", 0);";
            sqlite3_exec(db, q.str().c_str(), nullptr, nullptr, &errMsg);
        }
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg);
    }
}

int ParkingLot::findFreeSlotId() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id FROM slots WHERE occupied = 0 ORDER BY id LIMIT 1;", -1, &stmt, nullptr);
    int slotId = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        slotId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return slotId;
}

std::string ParkingLot::typeToString(VehicleType t) {
    switch (t) {
        case VehicleType::CAR: return "CAR";
        case VehicleType::BIKE: return "BIKE";
        case VehicleType::TRUCK: return "TRUCK";
    }
    return "CAR";
}

VehicleType ParkingLot::stringToType(const std::string& s) {
    if (s == "BIKE") return VehicleType::BIKE;
    if (s == "TRUCK") return VehicleType::TRUCK;
    return VehicleType::CAR;
}

double ParkingLot::calculateFee(VehicleType type, long durationMinutes) {
    double ratePerHour;
    switch (type) {
        case VehicleType::BIKE:  ratePerHour = 10.0; break;
        case VehicleType::CAR:   ratePerHour = 30.0; break;
        case VehicleType::TRUCK: ratePerHour = 60.0; break;
        default: ratePerHour = 30.0;
    }
    long hours = (durationMinutes + 59) / 60; 
    if (hours < 1) hours = 1;                 
    return hours * ratePerHour;
}

ParkResult ParkingLot::parkVehicle(const std::string& vehicleNumber, VehicleType type) {
    sqlite3_stmt* checkStmt;
    sqlite3_prepare_v2(db, "SELECT id FROM slots WHERE vehicle_number = ? AND occupied = 1;", -1, &checkStmt, nullptr);
    sqlite3_bind_text(checkStmt, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
        sqlite3_finalize(checkStmt);
        return {false, "Vehicle is already parked.", -1};
    }
    sqlite3_finalize(checkStmt);

    int slotId = findFreeSlotId();
    if (slotId == -1) {
        return {false, "Parking lot is full.", -1};
    }

    long now = static_cast<long>(std::time(nullptr));
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "UPDATE slots SET occupied = 1, vehicle_number = ?, type = ?, entry_time = ? WHERE id = ?;",
        -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, typeToString(type).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now);
    sqlite3_bind_int(stmt, 4, slotId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_stmt* hist;
    sqlite3_prepare_v2(db,
        "INSERT INTO history (vehicle_number, slot_id, type, entry_time) VALUES (?, ?, ?, ?);",
        -1, &hist, nullptr);
    sqlite3_bind_text(hist, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(hist, 2, slotId);
    sqlite3_bind_text(hist, 3, typeToString(type).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(hist, 4, now);
    sqlite3_step(hist);
    sqlite3_finalize(hist);

    return {true, "Vehicle parked successfully.", slotId};
}

ReleaseResult ParkingLot::releaseVehicle(const std::string& vehicleNumber) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT id, type, entry_time FROM slots WHERE vehicle_number = ? AND occupied = 1;",
        -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return {false, "Vehicle not found in parking lot.", 0.0, 0};
    }

    int slotId = sqlite3_column_int(stmt, 0);
    std::string typeStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    long entryTime = sqlite3_column_int64(stmt, 2);
    sqlite3_finalize(stmt);

    long now = static_cast<long>(std::time(nullptr));
    long durationMinutes = (now - entryTime) / 60;
    double fee = calculateFee(stringToType(typeStr), durationMinutes);

    sqlite3_stmt* update;
    sqlite3_prepare_v2(db,
        "UPDATE slots SET occupied = 0, vehicle_number = '', type = '', entry_time = 0 WHERE id = ?;",
        -1, &update, nullptr);
    sqlite3_bind_int(update, 1, slotId);
    sqlite3_step(update);
    sqlite3_finalize(update);
    
    sqlite3_stmt* hist;
    sqlite3_prepare_v2(db,
        "UPDATE history SET exit_time = ?, amount = ? "
        "WHERE vehicle_number = ? AND exit_time IS NULL "
        "ORDER BY id DESC LIMIT 1;",
        -1, &hist, nullptr);
    sqlite3_bind_int64(hist, 1, now);
    sqlite3_bind_double(hist, 2, fee);
    sqlite3_bind_text(hist, 3, vehicleNumber.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(hist);
    sqlite3_finalize(hist);

    return {true, "Vehicle released successfully.", fee, durationMinutes};
}

std::vector<Slot> ParkingLot::getOccupiedSlots() {
    std::vector<Slot> result;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT id, occupied, vehicle_number, type, entry_time FROM slots WHERE occupied = 1 ORDER BY id;",
        -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Slot s;
        s.id = sqlite3_column_int(stmt, 0);
        s.occupied = sqlite3_column_int(stmt, 1) != 0;
        s.vehicleNumber = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        s.type = stringToType(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        s.entryTimeEpoch = sqlite3_column_int64(stmt, 4);
        result.push_back(s);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Slot> ParkingLot::getAllSlots() {
    std::vector<Slot> result;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT id, occupied, vehicle_number, type, entry_time FROM slots ORDER BY id;",
        -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Slot s;
        s.id = sqlite3_column_int(stmt, 0);
        s.occupied = sqlite3_column_int(stmt, 1) != 0;
        const unsigned char* vn = sqlite3_column_text(stmt, 2);
        s.vehicleNumber = vn ? reinterpret_cast<const char*>(vn) : "";
        const unsigned char* tp = sqlite3_column_text(stmt, 3);
        s.type = stringToType(tp ? reinterpret_cast<const char*>(tp) : "CAR");
        s.entryTimeEpoch = sqlite3_column_int64(stmt, 4);
        result.push_back(s);
    }
    sqlite3_finalize(stmt);
    return result;
}

int ParkingLot::getFreeSlotCount() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM slots WHERE occupied = 0;", -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count;
}

std::string ParkingLot::getHistoryJson() {
    std::ostringstream out;
    out << "[";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "SELECT vehicle_number, slot_id, type, entry_time, exit_time, amount "
        "FROM history ORDER BY id DESC LIMIT 100;",
        -1, &stmt, nullptr);
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) out << ",";
        first = false;
        std::string vn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int slotId = sqlite3_column_int(stmt, 1);
        std::string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        long entry = sqlite3_column_int64(stmt, 3);
        bool hasExit = sqlite3_column_type(stmt, 4) != SQLITE_NULL;
        long exit = hasExit ? sqlite3_column_int64(stmt, 4) : 0;
        double amount = hasExit ? sqlite3_column_double(stmt, 5) : 0.0;

        out << "{"
            << "\"vehicle_number\":\"" << vn << "\","
            << "\"slot_id\":" << slotId << ","
            << "\"type\":\"" << type << "\","
            << "\"entry_time\":" << entry << ","
            << "\"exit_time\":" << (hasExit ? std::to_string(exit) : "null") << ","
            << "\"amount\":" << amount << ","
            << "\"status\":\"" << (hasExit ? "COMPLETED" : "ACTIVE") << "\""
            << "}";
    }
    sqlite3_finalize(stmt);
    out << "]";
    return out.str();
}
