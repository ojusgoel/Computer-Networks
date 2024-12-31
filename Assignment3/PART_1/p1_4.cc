#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

/**
 * \brief Prints flow statistics gathered by the Flow Monitor module.
 * \param flowmon_helper The Flow Monitor object gathering statistics
 * \param perFlowInfo If true then prints flow based statistics otherwise only prints aggregate statistics.
 */
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
       std::cout << "  Throughput: " << ( ((double)flow->second.rxBytes * 8) / (flow->second.timeLastRxPacket.GetSeconds ()- flow->second.timeFirstTxPacket.GetSeconds())/1024 ) << " Kbps" << std::endl;
       std::cout << "  Mean{Delay}: " << (flow->second.delaySum.GetSeconds()/flow->second.rxPackets) << std::endl;
       std::cout << "  Mean{Jitter}: " << (flow->second.jitterSum.GetSeconds()/(flow->second.rxPackets)) << std::endl;
     }


   }

     std::cout<< "Total throughput of System: "<<
     (totalBytesReceived)/totalTimeReceiving/1024<<" Kbps "<<std::endl;
     std::cout<<"Total packets transmitted: "<<totalPacketsTransmitted<<std::endl;
     std::cout<<"Total packets received: "<< totalPacketsReceived<<std::endl;
     std::cout<<"Total packets dropped: "<< totalPacketsDropped<<std::endl;
     std::cout << "Packet Lost Ratio: " << totalPacketsDropped / (double) (totalPacketsReceived + totalPacketsDropped) << std::endl;
}

int main (int argc, char *argv[]) {
    
    // Creating 2 ndoes
    NodeContainer nodes;
    nodes.Create(2);

    // Setting up p2p link
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer device = pointToPoint.Install(nodes);

    // Installing Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assigning IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(device);

    // Creating applications

    // First UDP Server on Node 2, listening on port 9
    uint16_t port1 = 9; // UDP port for first client-server pair
    UdpServerHelper server1(port1);
    ApplicationContainer serverApp1 = server1.Install(nodes.Get(1));
    serverApp1.Start(Seconds(1.0));
    serverApp1.Stop(Seconds(7.0));

    // First UDP Client on Node 1, sending to port 9
    OnOffHelper client1("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port1));
    client1.SetConstantRate(DataRate("1Mbps")); // Set Rate1 here
    client1.SetAttribute("PacketSize", UintegerValue(1024)); // Packet size
    ApplicationContainer clientApp1 = client1.Install(nodes.Get(0));
    clientApp1.Start(Seconds(2.0));
    clientApp1.Stop(Seconds(7.0));

    // Second UDP Server on Node 2, listening on a different port (10)
    uint16_t port2 = 10; // UDP port for the second client-server pair
    UdpServerHelper server2(port2);
    ApplicationContainer serverApp2 = server2.Install(nodes.Get(1));
    serverApp2.Start(Seconds(5.0));
    serverApp2.Stop(Seconds(10.0));

    // Second UDP Client on Node 1, sending to port 10
    OnOffHelper client2("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port2));
    client2.SetConstantRate(DataRate("1Mbps")); // Set Rate2 here
    client2.SetAttribute("PacketSize", UintegerValue(1024)); // Packet size
    ApplicationContainer clientApp2 = client2.Install(nodes.Get(0));
    clientApp2.Start(Seconds(6.0));
    clientApp2.Stop(Seconds(10.0));


    Simulator::Stop (Seconds (20.0)); 

    // Enabling flow monitor
    Ptr<FlowMonitor> flowmon;
    FlowMonitorHelper flowmonHelper;
    flowmon = flowmonHelper.InstallAll ();

    // Enable netanim
    AnimationInterface anim("p1_4_netanim.xml");    

    // Run the simulation, print the stats and destroy the simulators
    Simulator::Run ();
    printStats (flowmonHelper, true);
    Simulator::Destroy ();
    return 0;
}
