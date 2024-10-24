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
    // Habilitar logs para depuración
    LogComponentEnable("SwitchSimulation", LOG_LEVEL_INFO);
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    
    // Configuración del tiempo
    Time::SetResolution (Time::NS);

    // Crear los nodos para los dispositivos
    NodeContainer endDevices;
    endDevices.Create(6);  // 6 dispositivos finales

    // Crear los nodos para los switches
    NodeContainer switches;
    switches.Create(3);  // 3 switches

    // Configurar CSMA con mayor tasa de datos y menor delay
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue(DataRate("1Gbps")));
    csma.SetChannelAttribute ("Delay", TimeValue(NanoSeconds(6560)));

    // Contenedores para los dispositivos de red
    NetDeviceContainer devicesSwitch1;
    NetDeviceContainer devicesSwitch2;
    NetDeviceContainer devicesSwitch3;

    // Conectar los dispositivos a los switches (2 dispositivos por switch)
    for (uint32_t i = 0; i < 2; i++) {
        // Conectar al Switch 1
        NodeContainer pairSwitch1;
        pairSwitch1.Add(endDevices.Get(i));
        pairSwitch1.Add(switches.Get(0));
        devicesSwitch1.Add(csma.Install(pairSwitch1));

        // Conectar al Switch 2
        NodeContainer pairSwitch2;
        pairSwitch2.Add(endDevices.Get(i + 2));
        pairSwitch2.Add(switches.Get(1));
        devicesSwitch2.Add(csma.Install(pairSwitch2));

        // Conectar al Switch 3
        NodeContainer pairSwitch3;
        pairSwitch3.Add(endDevices.Get(i + 4));
        pairSwitch3.Add(switches.Get(2));
        devicesSwitch3.Add(csma.Install(pairSwitch3));
    }

    // Conectar los switches entre sí
    NetDeviceContainer switchInterconnects;
    
    // Switch 1 <-> Switch 2
    NodeContainer switch1to2;
    switch1to2.Add(switches.Get(0));
    switch1to2.Add(switches.Get(1));
    switchInterconnects.Add(csma.Install(switch1to2));

    // Switch 2 <-> Switch 3
    NodeContainer switch2to3;
    switch2to3.Add(switches.Get(1));
    switch2to3.Add(switches.Get(2));
    switchInterconnects.Add(csma.Install(switch2to3));

    // Switch 1 <-> Switch 3
    NodeContainer switch1to3;
    switch1to3.Add(switches.Get(0));
    switch1to3.Add(switches.Get(2));
    switchInterconnects.Add(csma.Install(switch1to3));

    // Instalar la pila de Internet
    InternetStackHelper internet;
    internet.Install(endDevices);
    internet.Install(switches);

    // Asignar direcciones IP
    Ipv4AddressHelper ipv4;
    
    // Para dispositivos conectados al Switch 1
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces1 = ipv4.Assign(devicesSwitch1);

    // Para dispositivos conectados al Switch 2
    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces2 = ipv4.Assign(devicesSwitch2);

    // Para dispositivos conectados al Switch 3
    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces3 = ipv4.Assign(devicesSwitch3);

    // Para las conexiones entre switches
    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer interfacesSwitches = ipv4.Assign(switchInterconnects);

    // Configurar enrutamiento global
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configurar aplicaciones TCP
    uint16_t port = 50000;

    // Crear servidores TCP en los primeros tres dispositivos
    ApplicationContainer sinkApps;
    for (uint32_t i = 0; i < 3; i++) {
        PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), port + i));
        sinkApps.Add(packetSinkHelper.Install(endDevices.Get(i)));
    }
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(20.0));

    // Crear clientes TCP en los últimos tres dispositivos
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

    // Habilitar pcap en todos los nodos
    csma.EnablePcap("switch-simulation", endDevices, true);
    csma.EnablePcap("switch-simulation-switches", switches, true);

    // Imprimir información de depuración
    NS_LOG_INFO("Simulation setup completed");
    NS_LOG_INFO("Number of nodes: " << endDevices.GetN() + switches.GetN());
    
    // Configurar el tiempo de simulación
    Simulator::Stop(Seconds(20.0));

    // Ejecutar la simulación
    NS_LOG_INFO("Starting simulation...");
    Simulator::Run();
    NS_LOG_INFO("Simulation completed");
    
    Simulator::Destroy();
    return 0;
}
