#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SwitchSimulation");

int main (int argc, char *argv[])
{
    
    LogComponentEnable("SwitchSimulation", LOG_LEVEL_INFO);
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    
    Time::SetResolution (Time::NS);

    NodeContainer endDevices;
    endDevices.Create(6);

    NodeContainer switches;
    switches.Create(3);

    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue(DataRate("1Gbps")));
    csma.SetChannelAttribute ("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer devicesSwitch1;
    NetDeviceContainer devicesSwitch2;
    NetDeviceContainer devicesSwitch3;

    for (uint32_t i = 0; i < 2; i++) {
        
        NodeContainer pairSwitch1;
        pairSwitch1.Add(endDevices.Get(i));
        pairSwitch1.Add(switches.Get(0));
        devicesSwitch1.Add(csma.Install(pairSwitch1));

        NodeContainer pairSwitch2;
        pairSwitch2.Add(endDevices.Get(i + 2));
        pairSwitch2.Add(switches.Get(1));
        devicesSwitch2.Add(csma.Install(pairSwitch2));

        NodeContainer pairSwitch3;
        pairSwitch3.Add(endDevices.Get(i + 4));
        pairSwitch3.Add(switches.Get(2));
        devicesSwitch3.Add(csma.Install(pairSwitch3));
    }

    NetDeviceContainer switchInterconnects;
    
    NodeContainer switch1to2;
    switch1to2.Add(switches.Get(0));
    switch1to2.Add(switches.Get(1));
    switchInterconnects.Add(csma.Install(switch1to2));

    NodeContainer switch2to3;
    switch2to3.Add(switches.Get(1));
    switch2to3.Add(switches.Get(2));
    switchInterconnects.Add(csma.Install(switch2to3));

    NodeContainer switch1to3;
    switch1to3.Add(switches.Get(0));
    switch1to3.Add(switches.Get(2));
    switchInterconnects.Add(csma.Install(switch1to3));

    InternetStackHelper internet;
    internet.Install(endDevices);
    internet.Install(switches);

    Ipv4AddressHelper ipv4;
    
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces1 = ipv4.Assign(devicesSwitch1);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces2 = ipv4.Assign(devicesSwitch2);

    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces3 = ipv4.Assign(devicesSwitch3);

    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesSwitches = ipv4.Assign(switchInterconnects);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 50000;

    ApplicationContainer sinkApps;
    for (uint32_t i = 0; i < 3; i++) {
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), port + i));
        sinkApps.Add(packetSinkHelper.Install(endDevices.Get(i)));
    }
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(20.0));

    ApplicationContainer sourceApps;
    for (uint32_t i = 0; i < 3; i++) {
        BulkSendHelper source("ns3::TcpSocketFactory",
            InetSocketAddress(interfaces1.GetAddress(i), port + i));
        source.SetAttribute("MaxBytes", UintegerValue(0));
        source.SetAttribute("SendSize", UintegerValue(1000));
        ApplicationContainer tempApp = source.Install(endDevices.Get(i + 3));
        sourceApps.Add(tempApp);
    }
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(19.0));

    csma.EnablePcap("switch-simulation", endDevices, true);
    csma.EnablePcap("switch-simulation-switches", switches, true);

    NS_LOG_INFO("Simulation setup completed");
    NS_LOG_INFO("Number of nodes: " << endDevices.GetN() + switches.GetN());
    
    Simulator::Stop(Seconds(20.0));

    NS_LOG_INFO("Starting simulation...");
    Simulator::Run();
    NS_LOG_INFO("Simulation completed");
    
    Simulator::Destroy();
    return 0;
}
