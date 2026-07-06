#include "crow.h"
#include "ParkingLot.hpp"
#include <sstream>

std::string slotToJson(const Slot& s) {
    std::ostringstream out;
    out << "{"
        << "\"id\":" << s.id << ","
        << "\"occupied\":" << (s.occupied ? "true" : "false") << ","
        << "\"vehicle_number\":\"" << s.vehicleNumber << "\","
        << "\"type\":\"" << ParkingLot::typeToString(s.type) << "\","
        << "\"entry_time\":" << s.entryTimeEpoch
        << "}";
    return out.str();
}

int main() {
    ParkingLot lot("parking.db", 30);

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]() {
        crow::response res;
        res.set_static_file_info("frontend/index.html");
        return res;
    });

    CROW_ROUTE(app, "/api/status")
    ([&lot]() {
        auto slots = lot.getAllSlots();
        std::ostringstream out;
        out << "{\"free_slots\":" << lot.getFreeSlotCount()
            << ",\"total_slots\":" << slots.size()
            << ",\"slots\":[";
        for (size_t i = 0; i < slots.size(); ++i) {
            if (i > 0) out << ",";
            out << slotToJson(slots[i]);
        }
        out << "]}";
        crow::response res(out.str());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/park").methods("POST"_method)
    ([&lot](const crow::request& req) {
        auto body = crow::json::load(req.body);
        crow::response res;
        res.set_header("Content-Type", "application/json");

        if (!body || !body.has("vehicle_number")) {
            res.code = 400;
            res.write("{\"success\":false,\"message\":\"vehicle_number is required\"}");
            return res;
        }

        std::string vehicleNumber = body["vehicle_number"].s();
        std::string typeStr = body.has("type") ? std::string(body["type"].s()) : "CAR";
        VehicleType type = ParkingLot::stringToType(typeStr);

        ParkResult result = lot.parkVehicle(vehicleNumber, type);

        std::ostringstream out;
        out << "{\"success\":" << (result.success ? "true" : "false")
            << ",\"message\":\"" << result.message << "\""
            << ",\"slot_id\":" << result.slotId << "}";
        res.code = result.success ? 200 : 409;
        res.write(out.str());
        return res;
    });

    CROW_ROUTE(app, "/api/release").methods("POST"_method)
    ([&lot](const crow::request& req) {
        auto body = crow::json::load(req.body);
        crow::response res;
        res.set_header("Content-Type", "application/json");

        if (!body || !body.has("vehicle_number")) {
            res.code = 400;
            res.write("{\"success\":false,\"message\":\"vehicle_number is required\"}");
            return res;
        }

        std::string vehicleNumber = body["vehicle_number"].s();
        ReleaseResult result = lot.releaseVehicle(vehicleNumber);

        std::ostringstream out;
        out << "{\"success\":" << (result.success ? "true" : "false")
            << ",\"message\":\"" << result.message << "\""
            << ",\"amount\":" << result.amount
            << ",\"duration_minutes\":" << result.durationMinutes << "}";
        res.code = result.success ? 200 : 404;
        res.write(out.str());
        return res;
    });

    CROW_ROUTE(app, "/api/history")
    ([&lot]() {
        crow::response res(lot.getHistoryJson());
        res.set_header("Content-Type", "application/json");
        return res;
    });

    app.port(18080).multithreaded().run();
    return 0;
}
