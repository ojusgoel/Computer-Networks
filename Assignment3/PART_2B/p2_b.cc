#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-module.h"
#include "fstream"
#include "ns3/error-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("P2B");

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
     totalTimeReceiving += flow->second.timeLastRxPacket.GetSeconds ();
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
       std::cout << "  Throughput: " << ( ((double)flow->second.rxBytes * 8) / (flow->second.timeLastRxPacket.GetSeconds ()) ) << "bits/s" << std::endl;
       std::cout << "  Mean{Delay}: " << (flow->second.delaySum.GetSeconds()/flow->second.rxPackets) << std::endl;
       std::cout << "  Mean{Jitter}: " << (flow->second.jitterSum.GetSeconds()/(flow->second.rxPackets)) << std::endl;
     }


   }

     std::cout<< "Total throughput of System: "<<
     (totalBytesReceived)/totalTimeReceiving<<" bps "<<std::endl;
     std::cout<<"Total packets transmitted: "<<totalPacketsTransmitted<<std::endl;
     std::cout<<"Total packets received: "<< totalPacketsReceived<<std::endl;
     std::cout<<"Total packets dropped: "<< totalPacketsDropped<<std::endl;
     std::cout << "Packet Lost Ratio: " << totalPacketsDropped / (double) (totalPacketsReceived + totalPacketsDropped) << std::endl;
     std::cout << "Packet Delivery Ratio (PDR): "
              << (double)totalPacketsReceived / totalPacketsTransmitted << std::endl;
}

int main(int argc, char *argv[]) {         

    double error = 0.2;
    uint32_t rngSeed = 1;
    std::string errorUnit = "packet";
    ns3::RateErrorModel::ErrorUnit errorUnitEnum = ns3::RateErrorModel::ERROR_UNIT_PACKET;
    // Enable logging for this component
    LogComponentEnable("P2B", LOG_LEVEL_ERROR);

    CommandLine cmd;
    cmd.AddValue("RngRun", "Seed for the random number generator", rngSeed);
    cmd.AddValue("error", "Error rate of the error model", error);
    cmd.AddValue("errorUnit", "Error unit of the error model", errorUnit);
    cmd.Parse(argc, argv);

    if (errorUnit == "byte") {        
        errorUnitEnum = ns3::RateErrorModel::ERROR_UNIT_BYTE;
    } else if (errorUnit == "bit") {
        errorUnitEnum = ns3::RateErrorModel::ERROR_UNIT_BIT;
    }else if (errorUnit == "packet") {
        errorUnitEnum = ns3::RateErrorModel::ERROR_UNIT_PACKET;
    }
    else {        
        NS_FATAL_ERROR("Invalid error unit specified. Correct values are: byte, bit, packet");        
    }
    
    // Setting the random number generator seed value
    RngSeedManager::SetSeed(rngSeed);

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
    positionAlloc->Add(Vector(0.0, 10.0, 0.0)); // n0
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

    // Enabling tracing
    pointToPoint.EnablePcapAll("p2_b");

    // Creating UdpEchoServer applications for n7 and n4
    UdpEchoServerHelper echoServer(9); 
    ApplicationContainer serverApps = echoServer.Install(NodesP2.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(11.0));

    // Creating UdpEchoClient applications
    UdpEchoClientHelper echoClient(InterfacesP2.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10));       
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(500.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(NodesP3.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(11.0));

    // Creating UdpEchoServer applications for n1 and n5
    UdpEchoServerHelper echoServer3(12); 
    ApplicationContainer serverApps3 = echoServer3.Install(NodesP2.Get(2));
    serverApps3.Start(Seconds(1.0));
    serverApps3.Stop(Seconds(10.0));

    // Creating UdpEchoClient applications
    UdpEchoClientHelper echoClient3(InterfacesP1.GetAddress(2), 12);
    echoClient3.SetAttribute("MaxPackets", UintegerValue(30));      
    echoClient3.SetAttribute("Interval", TimeValue(MilliSeconds(500.0)));
    echoClient3.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps3 = echoClient3.Install(NodesP1.Get(1));
    clientApps3.Start(Seconds(2.0));
    clientApps3.Stop(Seconds(10.0));

    // Creating UdpEchoServer applications for n2 and n6
    UdpEchoServerHelper echoServer1(10); 
    ApplicationContainer serverApps1 = echoServer1.Install(NodesP3.Get(0));
    serverApps1.Start(Seconds(1.0));
    serverApps1.Stop(Seconds(10.0));

    // Creating UdpEchoClient applications
    UdpEchoClientHelper echoClient1(InterfacesP3.GetAddress(0), 10); 
    echoClient1.SetAttribute("MaxPackets", UintegerValue(30));       
    echoClient1.SetAttribute("Interval", TimeValue(MilliSeconds(500.0)));
    echoClient1.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps1 = echoClient1.Install(NodesP1.Get(2));
    clientApps1.Start(Seconds(2.0));
    clientApps1.Stop(Seconds(10.0));

    // Creating UdpEchoServer applications From n0 to n3
    UdpEchoServerHelper echoServer2(11); 
    ApplicationContainer serverApps2 = echoServer2.Install(NodesP2.Get(0));
    serverApps2.Start(Seconds(1.0));
    serverApps2.Stop(Seconds(10.0));

    // Creating UdpEchoClient applications
    UdpEchoClientHelper echoClient2(InterfacesP2.GetAddress(0), 11); 
    echoClient2.SetAttribute("MaxPackets", UintegerValue(30));       
    echoClient2.SetAttribute("Interval", TimeValue(MilliSeconds(500.0)));
    echoClient2.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps2 = echoClient2.Install(NodesP1.Get(0));
    clientApps2.Start(Seconds(2.0));
    clientApps2.Stop(Seconds(10.0));

    // Defining the error model
    Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel>();
    errorModel->SetAttribute("ErrorRate",DoubleValue(error)); 
    errorModel->SetAttribute("ErrorUnit",EnumValue(errorUnitEnum)); 

    // Installing and applying error model on the links and devices
    
    for (uint32_t i = 0; i < DevicesP1.GetN(); ++i){
        DevicesP1.Get(i)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));
    }

    for (uint32_t i = 0; i < DevicesP2.GetN(); ++i){
        DevicesP2.Get(i)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));
    }

    for (uint32_t i = 0; i < DevicesP3.GetN(); ++i){
        DevicesP3.Get(i)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));
    }

    Simulator::Stop (Seconds (100.0));

    AnimationInterface anim("p2_b.xml");
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Running the simulation, printing the statistics and destroying the simulator.
    Simulator::Run();
    printStats (flowHelper, true);
    monitor->SerializeToXmlFile("p2_b_flow_mon.xml", true, true);
    Simulator::Destroy();

    return 0;
}
