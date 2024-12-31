/*create a dumbbell topology of 4 leaves. Create two TCP and two UDP flows between the left and right nodes, with defined data rates and packet sizes and ensure the bottleneck link is 2 Mbps. Enable tracing, generate PCAP files for packet capture, and output a NetAnim trace file for visualization. Routing tables of the routers should be printed, and the simulation should run for 10 seconds.*/



#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
    // Set the default time resolution
    Time::SetResolution(Time::NS);

    // Create 4 nodes on the left side
    NodeContainer leftNodes;
    leftNodes.Create(4);

    // Create 2 routers in the middle
    NodeContainer routers;
    routers.Create(2);

    // Create 4 nodes on the right side
    NodeContainer rightNodes;
    rightNodes.Create(4);

    // Point-to-Point link parameters for leaf-to-router connections
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // Install P2P links between the left nodes and the first router
    NetDeviceContainer leftDevices;
    for (uint32_t i = 0; i < leftNodes.GetN(); ++i) {
        NetDeviceContainer link = pointToPoint.Install(leftNodes.Get(i), routers.Get(0));
        leftDevices.Add(link);
    }

    // Set up the link between the two routers with a 2 Mbps data rate
    PointToPointHelper routerLink;
    routerLink.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    routerLink.SetChannelAttribute("Delay", StringValue("2ms"));

    // Install P2P link between the two routers
    NetDeviceContainer routerDevices = routerLink.Install(routers.Get(0), routers.Get(1));

    // Install P2P links between the right nodes and the second router
    NetDeviceContainer rightDevices;
    for (uint32_t i = 0; i < rightNodes.GetN(); ++i) {
        NetDeviceContainer link = pointToPoint.Install(rightNodes.Get(i), routers.Get(1));
        rightDevices.Add(link);
    }

    // Install Internet stack on all nodes
    InternetStackHelper stack;
    stack.Install(leftNodes);
    stack.Install(routers);
    stack.Install(rightNodes);

    // Assign IP addresses to the devices
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer leftInterfaces, routerInterfaces, rightInterfaces;

    for (uint32_t i = 0; i < leftNodes.GetN(); ++i) {
        std::ostringstream subnet;
        subnet << "10.1." << i + 1 << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.0");
        leftInterfaces.Add(address.Assign(leftDevices.Get(i * 2)));
        address.Assign(leftDevices.Get(i * 2 + 1));
    }

    address.SetBase("10.1.10.0", "255.255.255.0");
    routerInterfaces = address.Assign(routerDevices);

    for (uint32_t i = 0; i < rightNodes.GetN(); ++i) {
        std::ostringstream subnet;
        subnet << "10.2." << i + 1 << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.0");
        rightInterfaces.Add(address.Assign(rightDevices.Get(i * 2)));
        address.Assign(rightDevices.Get(i * 2 + 1));
    }

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Print routing table of the routers
    Ptr<OutputStreamWrapper> routingStream1 = Create<OutputStreamWrapper>(&std::cout);
    routers.Get(0)->GetObject<Ipv4>()->GetRoutingProtocol()->PrintRoutingTable(routingStream1);

    Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper>(&std::cout);
    routers.Get(1)->GetObject<Ipv4>()->GetRoutingProtocol()->PrintRoutingTable(routingStream2);

    // Enable tracing
    pointToPoint.EnablePcapAll("dumbbell-topology");

    // Create TCP traffic flows
    uint16_t tcpPort1 = 8080;
    uint16_t tcpPort2 = 8081;
    
    // TCP Flow 1 (leftNode[0] -> rightNode[0])
    Address tcpSinkAddress1(InetSocketAddress(rightInterfaces.GetAddress(0), tcpPort1));
    PacketSinkHelper tcpSinkHelper1("ns3::TcpSocketFactory", tcpSinkAddress1);
    ApplicationContainer tcpSinkApp1 = tcpSinkHelper1.Install(rightNodes.Get(0));
    tcpSinkApp1.Start(Seconds(1.0));
    tcpSinkApp1.Stop(Seconds(10.0));

    OnOffHelper tcpClient1("ns3::TcpSocketFactory", tcpSinkAddress1);
    tcpClient1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClient1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    tcpClient1.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
    tcpClient1.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer tcpClientApp1 = tcpClient1.Install(leftNodes.Get(0));
    tcpClientApp1.Start(Seconds(2.0));
    tcpClientApp1.Stop(Seconds(10.0));

    // TCP Flow 2 (leftNode[1] -> rightNode[1])
    Address tcpSinkAddress2(InetSocketAddress(rightInterfaces.GetAddress(1), tcpPort2));
    PacketSinkHelper tcpSinkHelper2("ns3::TcpSocketFactory", tcpSinkAddress2);
    ApplicationContainer tcpSinkApp2 = tcpSinkHelper2.Install(rightNodes.Get(1));
    tcpSinkApp2.Start(Seconds(1.0));
    tcpSinkApp2.Stop(Seconds(10.0));

    OnOffHelper tcpClient2("ns3::TcpSocketFactory", tcpSinkAddress2);
    tcpClient2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClient2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    tcpClient2.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
    tcpClient2.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer tcpClientApp2 = tcpClient2.Install(leftNodes.Get(1));
    tcpClientApp2.Start(Seconds(2.0));
    tcpClientApp2.Stop(Seconds(10.0));

    // Create UDP traffic flows yo
    uint16_t udpPort1 = 8082;
    uint16_t udpPort2 = 8083;

    // UDP Flow 1 (leftNode[2] -> rightNode[2])
    Address udpSinkAddress1(InetSocketAddress(rightInterfaces.GetAddress(2), udpPort1));
    PacketSinkHelper udpSinkHelper1("ns3::UdpSocketFactory", udpSinkAddress1);
    ApplicationContainer udpSinkApp1 = udpSinkHelper1.Install(rightNodes.Get(2));
    udpSinkApp1.Start(Seconds(1.0));
    udpSinkApp1.Stop(Seconds(10.0));

    OnOffHelper udpClient1("ns3::UdpSocketFactory", udpSinkAddress1);
    udpClient1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpClient1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    udpClient1.SetAttribute("DataRate", DataRateValue(DataRate("500kbps")));
    udpClient1.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer udpClientApp1 = udpClient1.Install(leftNodes.Get(2));
    udpClientApp1.Start(Seconds(2.0));
    udpClientApp1.Stop(Seconds(10.0));

    // UDP Flow 2 (leftNode[3] -> rightNode[3])
    Address udpSinkAddress2(InetSocketAddress(rightInterfaces.GetAddress(3), udpPort2));
    PacketSinkHelper udpSinkHelper2("ns3::UdpSocketFactory", udpSinkAddress2);
    ApplicationContainer udpSinkApp2 = udpSinkHelper2.Install(rightNodes.Get(3));
    udpSinkApp2.Start(Seconds(1.0));
    udpSinkApp2.Stop(Seconds(10.0));

    OnOffHelper udpClient2("ns3::UdpSocketFactory", udpSinkAddress2);
    udpClient2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpClient2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    udpClient2.SetAttribute("DataRate", DataRateValue(DataRate("500kbps")));
    udpClient2.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer udpClientApp2 = udpClient2.Install(leftNodes.Get(3));
    udpClientApp2.Start(Seconds(2.0));
    udpClientApp2.Stop(Seconds(10.0));
    
    //pointToPoint.BoundingBox (1, 1, 100, 100);

    // Set up NetAnim trace
    AnimationInterface anim("dumbbell-topology.xml");
    anim.EnablePacketMetadata (); // Optional
    anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10.0)); // Optional

    // Run the simulation
    Simulator::Run();
    Simulator::Destroy();
    
    return 0;
}
