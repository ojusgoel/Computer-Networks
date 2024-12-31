#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "fstream"
#include "ns3/mobility-module.h"

using namespace ns3;


void
printStats (FlowMonitorHelper &flowmon_helper, bool perFlowInfo) {
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon_helper.GetClassifier());
  std::string proto;
  Ptr<FlowMonitor> monitor = flowmon_helper.GetMonitor ();
  std::map < FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();
  double totalTimeReceiving;
  uint64_t totalPacketsReceived, totalPacketsDropped, totalBytesReceived,totalPacketsTransmitted;

  totalBytesReceived = 0, totalPacketsDropped = 0, totalPacketsReceived = 0, totalTimeReceiving = 0,totalPacketsTransmitted = 0;
  for (std::map< FlowId, FlowMonitor::FlowStats>::iterator flow = stats.begin(); flow != stats.end(); flow++)
  {
    Ipv4FlowClassifier::FiveTuple  t = classifier->FindFlow(flow->first);
    switch(t.protocol)
     {
     case(6):
         proto = "TCP";
         break;
     case(17):
         proto = "UDP";
         break;
     default:
         exit(1);
     }
     totalBytesReceived += (double) flow->second.rxBytes * 8;
     totalTimeReceiving += flow->second.timeLastRxPacket.GetSeconds ()-flow->second.timeFirstTxPacket.GetSeconds();
     totalPacketsReceived += flow->second.rxPackets;
     totalPacketsDropped += flow->second.txPackets - flow->second.rxPackets;
     totalPacketsTransmitted += flow->second.txPackets;
     if (perFlowInfo)
     {
       std::cout << "FlowID: " << flow->first << " (" << proto << " "
                 << t.sourceAddress << " / " << t.sourcePort << " --> "
                 << t.destinationAddress << " / " << t.destinationPort << ")" << std::endl;
       std::cout << "  Tx Bytes: " << flow->second.txBytes << std::endl;
       std::cout << "  Rx Bytes: " << flow->second.rxBytes << std::endl;
       std::cout << "  Tx Packets: " << flow->second.txPackets << std::endl;
       std::cout << "  Rx Packets: " << flow->second.rxPackets << std::endl;
       std::cout << "  Time LastRxPacket: " << flow->second.timeLastRxPacket.GetSeconds () << "s" << std::endl;
       std::cout << "  Lost Packets: " << flow->second.lostPackets << std::endl;
       std::cout << "  Pkt Lost Ratio: " << ((double)flow->second.txPackets-(double)flow->second.rxPackets)/(double)flow->second.txPackets << std::endl;
       std::cout << "  Throughput: " << ( ((double)flow->second.rxBytes * 8) / (flow->second.timeLastRxPacket.GetSeconds ()-flow->second.timeFirstTxPacket.GetSeconds())/1024/1024 ) << " Mbps" << std::endl;
       std::cout << "  Mean{Delay}: " << (flow->second.delaySum.GetSeconds()/flow->second.rxPackets) << std::endl;
       std::cout << "  Mean{Jitter}: " << (flow->second.jitterSum.GetSeconds()/(flow->second.rxPackets)) << std::endl;
     }


   }

     std::cout<< "Total throughput of System: "<<
     (totalBytesReceived)/totalTimeReceiving/1024/1024<<" Mbps "<<std::endl;
     std::cout<<"Total packets transmitted: "<<totalPacketsTransmitted<<std::endl;
     std::cout<<"Total packets received: "<< totalPacketsReceived<<std::endl;
     std::cout<<"Total packets dropped: "<< totalPacketsDropped<<std::endl;
     std::cout << "Packet Lost Ratio: " << totalPacketsDropped / (double) (totalPacketsReceived + totalPacketsDropped) << std::endl;
}

int main(int argc, char *argv[]) {
    // Setting the default time resolution
    Time::SetResolution(Time::NS);

    // Creating nodes
    NodeContainer NodesP1, routers, NodesP2, NodesP3;
    NodesP1.Create(3);  // Nodes n0, n1, n2
    NodesP2.Create(3);  // Nodes n3, n4, n5
    NodesP3.Create(2);  // Nodes n6, n7
    routers.Create(3);  // Routers router0, router1, router2

    // Setting constant positions for all nodes (Previously we had error/warning for position of nodes,so correcting it this time)
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // Setting positions for NodesP1
    positionAlloc->Add(Vector(0.0, 10.0, 0.0));  // n0
    positionAlloc->Add(Vector(0.0, 20.0, 0.0)); // n1
    positionAlloc->Add(Vector(0.0, 30.0, 0.0)); // n2

    // Setting positions for NodesP2
    positionAlloc->Add(Vector(30.0, 0.0, 0.0));  // n3
    positionAlloc->Add(Vector(30.0, 10.0, 0.0)); // n4
    positionAlloc->Add(Vector(30.0, 20.0, 0.0)); // n5

    // Setting positions for NodesP3
    positionAlloc->Add(Vector(40.0, 30.0, 0.0)); // n6
    positionAlloc->Add(Vector(40.0, 40.0, 0.0)); // n7

    // Setting positions for routers
    positionAlloc->Add(Vector(10.0, 20.0, 0.0)); // router0
    positionAlloc->Add(Vector(20.0, 5.0, 0.0));  // router1
    positionAlloc->Add(Vector(20.0, 35.0, 0.0)); // router2

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Installing mobility model on all nodes
    mobility.Install(NodesP1);
    mobility.Install(NodesP2);
    mobility.Install(NodesP3);
    mobility.Install(routers);

    // Setting up Point-to-Point link parameters
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));
    
    // Installing P2P links from NodesP1 to router0
    NetDeviceContainer DevicesP1;
    for (uint32_t i = 0; i < NodesP1.GetN(); ++i) {
        NetDeviceContainer link = pointToPoint.Install(NodesP1.Get(i), routers.Get(0));
        DevicesP1.Add(link);
    }

    // Installing P2P links from NodesP2 to router1
    NetDeviceContainer DevicesP2;
    for (uint32_t i = 0; i < NodesP2.GetN(); ++i) {
        NetDeviceContainer link = pointToPoint.Install(NodesP2.Get(i), routers.Get(1));
        DevicesP2.Add(link);
    }

    // Installing P2P links from NodesP3 to router2
    NetDeviceContainer DevicesP3;
    for (uint32_t i = 0; i < NodesP3.GetN(); ++i) {
        NetDeviceContainer link = pointToPoint.Install(NodesP3.Get(i), routers.Get(2));
        DevicesP3.Add(link);
    }

    // Installing P2P links between routers (router0 to router1 and router2)
    PointToPointHelper routerLink;
    routerLink.SetDeviceAttribute("DataRate", StringValue("4Mbps"));
    routerLink.SetChannelAttribute("Delay", StringValue("40ms"));
    
    NetDeviceContainer routerLinks;
    routerLinks.Add(routerLink.Install(routers.Get(0), routers.Get(1)));
    routerLinks.Add(routerLink.Install(routers.Get(0), routers.Get(2)));

    // Installing Internet stack
    InternetStackHelper stack;
    stack.Install(NodesP1);
    stack.Install(NodesP2);
    stack.Install(NodesP3);
    stack.Install(routers);

    // Assigning IP addresses
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer InterfacesP1, InterfacesP2, InterfacesP3, routerInterfaces;

    // Assigning IP addresses to P1 nodes (n0, n1, n2) connected to router0
    for (uint32_t i = 0; i < 3; ++i) {
        std::ostringstream subnet;
        subnet << "10.1." << i + 1 << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.252");
        InterfacesP1.Add(address.Assign(DevicesP1.Get(i * 2)));
        address.Assign(DevicesP1.Get(i * 2 + 1));
    }

    // Assigning IP addresses to P2 nodes (n3, n4, n5) connected to router1
    for (uint32_t i = 0; i < 3; ++i) {
        std::ostringstream subnet;
        subnet << "10.2." << i + 1 << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.252");
        InterfacesP2.Add(address.Assign(DevicesP2.Get(i * 2)));  // Assign IP to node's device
        address.Assign(DevicesP2.Get(i * 2 + 1));  // Assign IP to router's device
    }

    // Assigning IP addresses to P3 nodes (n6, n7) connected to router2
    for (uint32_t i = 0; i < 2; ++i) {
        std::ostringstream subnet;
        subnet << "10.3." << i + 1 << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.252");
        InterfacesP3.Add(address.Assign(DevicesP3.Get(i * 2)));  // Assign IP to node's device
        address.Assign(DevicesP3.Get(i * 2 + 1));  // Assign IP to router's device
    }

    // Assigning IP addresses to router-router links
    for (uint32_t i = 0; i < 2; ++i) {
        std::ostringstream subnet;
        subnet << "10.4." << i + 1 << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.252");
        routerInterfaces.Add(address.Assign(routerLinks.Get(i*2)));
        address.Assign(routerLinks.Get(i*2+1));
    }

    // Populating routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Printing routing tables of routers
    Ptr<OutputStreamWrapper> routingStream1 = Create<OutputStreamWrapper>(&std::cout);
    routers.Get(0)->GetObject<Ipv4>()->GetRoutingProtocol()->PrintRoutingTable(routingStream1);

    Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper>(&std::cout);
    routers.Get(1)->GetObject<Ipv4>()->GetRoutingProtocol()->PrintRoutingTable(routingStream2);

    Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper>(&std::cout);
    routers.Get(2)->GetObject<Ipv4>()->GetRoutingProtocol()->PrintRoutingTable(routingStream3);

    // Generate routing tables file
    std::ofstream routingFile("p2_routing_tables.txt");
    for (uint32_t i = 0; i < routers.GetN(); ++i) {
        Ptr<Ipv4> ipv4 = routers.Get(i)->GetObject<Ipv4>();
        routingFile << "Router " << i << " Routing Table:\n";
        ipv4->GetRoutingProtocol()->PrintRoutingTable(Create<OutputStreamWrapper>(&routingFile));
        routingFile << "\n\n";
    }

    routingFile.close();

    // Enable tracing
    pointToPoint.EnablePcapAll("p2_a");

    // Create TCP traffic flows
    uint16_t tcpPort1 = 8080;
    uint16_t tcpPort2 = 8081;
    uint16_t tcpPort3 = 8082;

    // Creating UDP traffic flow
    uint16_t udpPort1 = 8084;

    // TCP Flow 1: From n7 to n4
    Address tcpSinkAddress1(InetSocketAddress(InterfacesP2.GetAddress(1), tcpPort1));
    PacketSinkHelper tcpSinkHelper1("ns3::TcpSocketFactory", tcpSinkAddress1);
    ApplicationContainer tcpSinkApp1 = tcpSinkHelper1.Install(NodesP2.Get(1));
    tcpSinkApp1.Start(Seconds(1.0));
    tcpSinkApp1.Stop(Seconds(11.0));

    OnOffHelper tcpClient1("ns3::TcpSocketFactory", tcpSinkAddress1);
    tcpClient1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClient1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    tcpClient1.SetAttribute("DataRate", DataRateValue(DataRate("0.5Mbps")));
    tcpClient1.SetAttribute("PacketSize", UintegerValue(2096));
    ApplicationContainer tcpClientApp1 = tcpClient1.Install(NodesP3.Get(1));
    tcpClientApp1.Start(Seconds(2.0));
    tcpClientApp1.Stop(Seconds(11.0));

    // UDP Flow: From n1 to n5
    Address udpSinkAddress1(InetSocketAddress(InterfacesP2.GetAddress(2), udpPort1));
    PacketSinkHelper udpSinkHelper1("ns3::UdpSocketFactory", udpSinkAddress1);
    ApplicationContainer udpSinkApp1 = udpSinkHelper1.Install(NodesP2.Get(2));
    udpSinkApp1.Start(Seconds(1.0));
    udpSinkApp1.Stop(Seconds(11.0));

    OnOffHelper udpClient("ns3::UdpSocketFactory", udpSinkAddress1);
    udpClient.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpClient.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    udpClient.SetAttribute("DataRate", DataRateValue(DataRate("4Mbps")));
    udpClient.SetAttribute("PacketSize", UintegerValue(512));
    ApplicationContainer udpClientApp = udpClient.Install(NodesP1.Get(1));
    udpClientApp.Start(Seconds(2.0));
    udpClientApp.Stop(Seconds(11.0));

    // TCP Flow 2: From n2 to n6
    Address tcpSinkAddress2(InetSocketAddress(InterfacesP3.GetAddress(0), tcpPort2));
    PacketSinkHelper tcpSinkHelper2("ns3::TcpSocketFactory", tcpSinkAddress2);
    ApplicationContainer tcpSinkApp2 = tcpSinkHelper2.Install(NodesP3.Get(0));
    tcpSinkApp2.Start(Seconds(1.0));
    tcpSinkApp2.Stop(Seconds(11.0));

    OnOffHelper tcpClient2("ns3::TcpSocketFactory", tcpSinkAddress2);
    tcpClient2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClient2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    tcpClient2.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
    tcpClient2.SetAttribute("PacketSize", UintegerValue(2096));
    ApplicationContainer tcpClientApp2 = tcpClient2.Install(NodesP1.Get(2));
    tcpClientApp2.Start(Seconds(2.0));
    tcpClientApp2.Stop(Seconds(11.0));

    // TCP Flow 3: From n0 to n3
    Address tcpSinkAddress3(InetSocketAddress(InterfacesP2.GetAddress(0), tcpPort3));
    PacketSinkHelper tcpSinkHelper3("ns3::TcpSocketFactory", tcpSinkAddress3);
    ApplicationContainer tcpSinkApp3 = tcpSinkHelper3.Install(NodesP2.Get(0));
    tcpSinkApp3.Start(Seconds(1.0));
    tcpSinkApp3.Stop(Seconds(11.0));

    OnOffHelper tcpClient3("ns3::TcpSocketFactory", tcpSinkAddress3);
    tcpClient3.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    tcpClient3.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    tcpClient3.SetAttribute("DataRate", DataRateValue(DataRate("3Mbps")));
    tcpClient3.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer tcpClientApp3 = tcpClient3.Install(NodesP1.Get(0));
    tcpClientApp3.Start(Seconds(2.0));
    tcpClientApp3.Stop(Seconds(11.0));

    Simulator::Stop (Seconds (20.0)); 

    // Setting up NetAnim
    AnimationInterface anim("p2.xml");

    // Creating FlowMonitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Running the simulation
    Simulator::Run();
    printStats (flowHelper, true);
    
    // Collecting throughput statistics
    monitor->SerializeToXmlFile("p2_flow_mon.xml", true, true);

    Simulator::Destroy();

    return 0;
}
