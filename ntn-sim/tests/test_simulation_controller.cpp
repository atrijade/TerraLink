#include <gtest/gtest.h>
#include "ntn-sim/core/simulation_controller.h"
#include "ntn-sim/core/event_scheduler.h"
#include "ntn-sim/api/rest_api.h"
#include "ntn-sim/orbit/orbit_engine.h"
#include "ntn-sim/routing/routing_engine.h"
#include "ntn-sim/gateway/gateway.h"
#include "ntn-sim/gateway/gateway_engine.h"
#include "ntn-sim/metrics/metrics_engine.h"
#include "ntn-sim/user/user_engine.h"
#include "ntn-sim/user/user.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "httplib.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

TEST(SimulationControllerTest, InitialState) {
    NtnSim::Core::SimulationController controller;
    EXPECT_EQ(controller.GetScheduler(), nullptr);
    
    controller.InitializeEngines();
    EXPECT_NE(controller.GetScheduler(), nullptr);
    EXPECT_NE(controller.GetOrbitEngine(), nullptr);
    EXPECT_NE(controller.GetUserEngine(), nullptr);
    EXPECT_NE(controller.GetCoverageEngine(), nullptr);
    EXPECT_NE(controller.GetSignalEngine(), nullptr);
    EXPECT_NE(controller.GetHandoverEngine(), nullptr);
    EXPECT_NE(controller.GetPacketEngine(), nullptr);
    EXPECT_NE(controller.GetRoutingEngine(), nullptr);
    EXPECT_NE(controller.GetGateway(), nullptr);
    EXPECT_NE(controller.GetGatewayEngine(), nullptr);
    EXPECT_NE(controller.GetMetricsEngine(), nullptr);
    EXPECT_NE(controller.GetRestApi(), nullptr);

    // Verify RestApi is wired to SimulationController and underlying engines
    auto* api = controller.GetRestApi();
    ASSERT_NE(api, nullptr);
    EXPECT_EQ(api->GetSimulationController(), &controller);
    EXPECT_EQ(api->GetOrbitEngine(), controller.GetOrbitEngine());
    EXPECT_EQ(api->GetRoutingEngine(), controller.GetRoutingEngine());
    EXPECT_EQ(api->GetGatewayEngine(), controller.GetGatewayEngine());
    EXPECT_EQ(api->GetGateway(), controller.GetGateway());
    EXPECT_EQ(api->GetMetricsEngine(), controller.GetMetricsEngine());
    EXPECT_EQ(api->GetUserEngine(), controller.GetUserEngine());
    EXPECT_EQ(api->GetCoverageEngine(), controller.GetCoverageEngine());
    EXPECT_EQ(api->GetPacketEngine(), controller.GetPacketEngine());
}

TEST(RestApiTest, DirectControllerWiringAndLifecycle) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    NtnSim::Api::RestApi api(&controller);
    EXPECT_EQ(api.GetSimulationController(), &controller);
    EXPECT_EQ(api.GetOrbitEngine(), controller.GetOrbitEngine());
    EXPECT_EQ(api.GetRoutingEngine(), controller.GetRoutingEngine());
    EXPECT_EQ(api.GetGatewayEngine(), controller.GetGatewayEngine());
    EXPECT_EQ(api.GetMetricsEngine(), controller.GetMetricsEngine());

    EXPECT_FALSE(api.IsRunning());
    EXPECT_TRUE(api.StartServer(9090));
    EXPECT_TRUE(api.IsRunning());
    EXPECT_EQ(api.GetPort(), 9090);

    api.StopServer();
    EXPECT_FALSE(api.IsRunning());
}

// =========================================================================
// Step 2: REST JSON Serializer Tests
// =========================================================================

TEST(RestApiTest, HandleGetSatellites_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    // Generate a mini Walker constellation (2 planes x 2 sats = 4 sats)
    controller.GetOrbitEngine()->GenerateWalkerConstellation(2, 2, 600.0, 53.0);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetSatellites();

    EXPECT_FALSE(json.empty());
    EXPECT_EQ(json.front(), '[');
    EXPECT_EQ(json.back(), ']');
    EXPECT_NE(json.find("\"id\":\"sat-p0-s0\""), std::string::npos);
    EXPECT_NE(json.find("\"lat\":"), std::string::npos);
    EXPECT_NE(json.find("\"lon\":"), std::string::npos);
    EXPECT_NE(json.find("\"altKm\":"), std::string::npos);
    EXPECT_NE(json.find("\"ecef\":"), std::string::npos);
    EXPECT_NE(json.find("\"velKms\":"), std::string::npos);
}

TEST(RestApiTest, HandleGetTopology_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    controller.GetRoutingEngine()->AddLink("sat-0", "sat-1", 2400.0e3, false, 10.0e9);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetTopology();

    EXPECT_NE(json.find("{\"links\":["), std::string::npos);
    EXPECT_NE(json.find("\"src\":\"sat-0\""), std::string::npos);
    EXPECT_NE(json.find("\"dst\":\"sat-1\""), std::string::npos);
    EXPECT_NE(json.find("\"distanceKm\":2400.000"), std::string::npos);
    EXPECT_NE(json.find("\"dataRateGbps\":10.000"), std::string::npos);
    EXPECT_NE(json.find("\"active\":true"), std::string::npos);
}

TEST(RestApiTest, HandleGetGateways_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    auto* gw = controller.GetGateway();
    ASSERT_NE(gw, nullptr);
    gw->ConnectSatellite("sat-feeder-1");
    gw->ReceivePacket(101, 0.020, 1500, 0.0);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetGateways();

    EXPECT_EQ(json.front(), '[');
    EXPECT_EQ(json.back(), ']');
    EXPECT_NE(json.find("\"id\":\"gw-0\""), std::string::npos);
    EXPECT_NE(json.find("\"connectedSat\":\"sat-feeder-1\""), std::string::npos);
    EXPECT_NE(json.find("\"hasActiveFeeder\":true"), std::string::npos);
    EXPECT_NE(json.find("\"throughputMbps\":"), std::string::npos);
    EXPECT_NE(json.find("\"jitterMs\":"), std::string::npos);
    EXPECT_NE(json.find("\"feederProfile\":"), std::string::npos);
}

TEST(RestApiTest, HandleGetUsers_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    auto user = std::make_shared<NtnSim::User::User>("ue-alpha", NtnSim::Utils::GeodeticPosition{37.7749, -122.4194, 0.0});
    user->SetServingSatelliteId("sat-serving-1");
    controller.GetUserEngine()->AddUser(user);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetUsers();

    EXPECT_EQ(json.front(), '[');
    EXPECT_EQ(json.back(), ']');
    EXPECT_NE(json.find("\"id\":\"ue-alpha\""), std::string::npos);
    EXPECT_NE(json.find("\"servingSat\":\"sat-serving-1\""), std::string::npos);
    EXPECT_NE(json.find("\"lat\":37.774900"), std::string::npos);
}

TEST(RestApiTest, HandleGetMetrics_Serialization) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    auto* metrics = controller.GetMetricsEngine();
    metrics->RecordPacketSent(1, 1500, 0.0);
    metrics->RecordPacketReceived(1, 0.025, 0.025);
    metrics->RecordHandover("ue-1", "sat-1", "sat-2", 0.05, true, 0.035);

    auto* api = controller.GetRestApi();
    std::string json = api->HandleGetMetrics();

    EXPECT_NE(json.find("\"simTime\":"), std::string::npos);
    EXPECT_NE(json.find("\"traffic\":{"), std::string::npos);
    EXPECT_NE(json.find("\"packetsSent\":1"), std::string::npos);
    EXPECT_NE(json.find("\"packetsDelivered\":1"), std::string::npos);
    EXPECT_NE(json.find("\"pdrPercent\":100.0000"), std::string::npos);
    EXPECT_NE(json.find("\"handovers\":{"), std::string::npos);
    EXPECT_NE(json.find("\"totalSuccesses\":1"), std::string::npos);
    EXPECT_NE(json.find("\"radio\":{"), std::string::npos);
}

TEST(RestApiTest, EmbeddedServer_StartStopLifecycle) {
    NtnSim::Api::RestApi api;
    EXPECT_FALSE(api.IsRunning());

    bool started = api.StartServer(18081);
    EXPECT_TRUE(started);
    EXPECT_TRUE(api.IsRunning());
    EXPECT_EQ(api.GetPort(), 18081);

    api.StopServer();
    EXPECT_FALSE(api.IsRunning());
}

TEST(RestApiTest, EmbeddedServer_HttpRequests) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();
    controller.GetOrbitEngine()->GenerateWalkerConstellation(2, 2, 600.0, 53.0);

    auto* api = controller.GetRestApi();
    ASSERT_NE(api, nullptr);

    bool started = api->StartServer(18082);
    ASSERT_TRUE(started);
    EXPECT_TRUE(api->IsRunning());

    // Connect using httplib client
    httplib::Client cli("127.0.0.1", 18082);
    cli.set_connection_timeout(2, 0);
    cli.set_read_timeout(2, 0);

    // 1. GET /health
    auto resHealth = cli.Get("/health");
    ASSERT_TRUE(resHealth != nullptr);
    EXPECT_EQ(resHealth->status, 200);
    EXPECT_NE(resHealth->body.find("\"status\": \"RUNNING\""), std::string::npos);

    // 2. GET /api/satellites
    auto resSats = cli.Get("/api/satellites");
    ASSERT_TRUE(resSats != nullptr);
    EXPECT_EQ(resSats->status, 200);
    EXPECT_NE(resSats->body.find("\"id\":\"sat-p0-s0\""), std::string::npos);

    // 3. POST /api/start
    auto resStart = cli.Post("/api/start");
    ASSERT_TRUE(resStart != nullptr);
    EXPECT_EQ(resStart->status, 200);
    EXPECT_NE(resStart->body.find("\"status\": \"STARTED\""), std::string::npos);

    // 4. POST /api/stop
    auto resStop = cli.Post("/api/stop");
    ASSERT_TRUE(resStop != nullptr);
    EXPECT_EQ(resStop->status, 200);
    EXPECT_NE(resStop->body.find("\"status\": \"STOPPED\""), std::string::npos);

    api->StopServer();
    EXPECT_FALSE(api->IsRunning());
}

TEST(RestApiTest, LinkFailureInjection_DirectAndJsonHandler) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    auto* routing = controller.GetRoutingEngine();
    ASSERT_NE(routing, nullptr);

    // Add a test link between sat-1 and sat-2
    routing->AddLink("sat-1", "sat-2", 1000000.0, false, 10e9, true);
    auto linkOpt = routing->GetLink("sat-1", "sat-2");
    ASSERT_TRUE(linkOpt.has_value());
    EXPECT_TRUE(linkOpt->isActive);

    auto* api = controller.GetRestApi();
    ASSERT_NE(api, nullptr);

    // 1. Direct failure injection
    bool okFail = api->InjectLinkFailure("sat-1", "sat-2", true);
    EXPECT_TRUE(okFail);
    linkOpt = routing->GetLink("sat-1", "sat-2");
    ASSERT_TRUE(linkOpt.has_value());
    EXPECT_FALSE(linkOpt->isActive);

    // 2. Direct link restoration
    bool okRestore = api->RestoreLink("sat-1", "sat-2", true);
    EXPECT_TRUE(okRestore);
    linkOpt = routing->GetLink("sat-1", "sat-2");
    ASSERT_TRUE(linkOpt.has_value());
    EXPECT_TRUE(linkOpt->isActive);

    // 3. Test HandlePostInjectLinkFailure JSON handler
    std::string failJson = api->HandlePostInjectLinkFailure(
        "{\"source\":\"sat-1\",\"target\":\"sat-2\",\"bidirectional\":true}"
    );
    EXPECT_NE(failJson.find("\"success\":true"), std::string::npos);
    EXPECT_NE(failJson.find("\"action\":\"LINK_FAILURE_INJECTED\""), std::string::npos);
    linkOpt = routing->GetLink("sat-1", "sat-2");
    ASSERT_TRUE(linkOpt.has_value());
    EXPECT_FALSE(linkOpt->isActive);

    // 4. Test HandlePostRestoreLink JSON handler
    std::string restoreJson = api->HandlePostRestoreLink(
        "{\"source\":\"sat-1\",\"target\":\"sat-2\"}"
    );
    EXPECT_NE(restoreJson.find("\"success\":true"), std::string::npos);
    EXPECT_NE(restoreJson.find("\"action\":\"LINK_RESTORED\""), std::string::npos);
    linkOpt = routing->GetLink("sat-1", "sat-2");
    ASSERT_TRUE(linkOpt.has_value());
    EXPECT_TRUE(linkOpt->isActive);

    // 5. Missing params handling
    std::string badJson = api->HandlePostInjectLinkFailure("{\"badKey\":\"none\"}");
    EXPECT_NE(badJson.find("\"success\":false"), std::string::npos);
}

TEST(RestApiTest, EmbeddedServer_LinkFailureRouteOverHttp) {
    NtnSim::Core::SimulationController controller;
    controller.InitializeEngines();

    auto* routing = controller.GetRoutingEngine();
    ASSERT_NE(routing, nullptr);
    routing->AddLink("sat-alpha", "sat-beta", 1500000.0, false, 10e9, true);

    auto* api = controller.GetRestApi();
    ASSERT_NE(api, nullptr);

    bool started = api->StartServer(18083);
    ASSERT_TRUE(started);
    EXPECT_TRUE(api->IsRunning());

    httplib::Client cli("127.0.0.1", 18083);
    cli.set_connection_timeout(2, 0);
    cli.set_read_timeout(2, 0);

    // 1. POST /api/inject/link-failure
    std::string failPayload = "{\"source\":\"sat-alpha\",\"target\":\"sat-beta\",\"bidirectional\":true}";
    auto resFail = cli.Post("/api/inject/link-failure", failPayload, "application/json");
    ASSERT_TRUE(resFail != nullptr);
    EXPECT_EQ(resFail->status, 200);
    EXPECT_NE(resFail->body.find("\"success\":true"), std::string::npos);
    EXPECT_NE(resFail->body.find("LINK_FAILURE_INJECTED"), std::string::npos);

    // Verify RoutingEngine state was updated
    auto linkOpt = routing->GetLink("sat-alpha", "sat-beta");
    ASSERT_TRUE(linkOpt.has_value());
    EXPECT_FALSE(linkOpt->isActive);

    // 2. POST /api/inject/link-restore
    std::string restorePayload = "{\"source\":\"sat-alpha\",\"target\":\"sat-beta\"}";
    auto resRestore = cli.Post("/api/inject/link-restore", restorePayload, "application/json");
    ASSERT_TRUE(resRestore != nullptr);
    EXPECT_EQ(resRestore->status, 200);
    EXPECT_NE(resRestore->body.find("\"success\":true"), std::string::npos);
    EXPECT_NE(resRestore->body.find("LINK_RESTORED"), std::string::npos);

    linkOpt = routing->GetLink("sat-alpha", "sat-beta");
    ASSERT_TRUE(linkOpt.has_value());
    EXPECT_TRUE(linkOpt->isActive);

    api->StopServer();
    EXPECT_FALSE(api->IsRunning());
}

