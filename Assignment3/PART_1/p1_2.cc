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
    pointToPoint.SetChannelAttribute("Delay", StringValue("1000ms"));

    NetDeviceContainer device = pointToPoint.Install(nodes);

    // Installing Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // Assigning IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(device);

    // Creating applications
    // Create a UDP Server on Node 2
    uint16_t port = 9; // UDP port
    UdpServerHelper server(port);
    ApplicationContainer serverApp = server.Install(nodes.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(10.0));

    // Create a UDP Client on Node 1 with a fixed data rate
    OnOffHelper client("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), port));
    client.SetConstantRate(DataRate("1Mbps")); // Set Rate1 here
    client.SetAttribute("PacketSize", UintegerValue(1024)); // Packet size

    ApplicationContainer clientApp = client.Install(nodes.Get(0));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(10.0));

    Simulator::Stop (Seconds (12.0)); 

    // Enabling flow monitor
    Ptr<FlowMonitor> flowmon;
    FlowMonitorHelper flowmonHelper;
    flowmon = flowmonHelper.InstallAll ();

    // Enable netanim
    AnimationInterface anim("p1_2_netanim_final.xml");    

    // Run the simulation, print the stats and destroy the simulators
    Simulator::Run ();
    printStats (flowmonHelper, true);
    Simulator::Destroy ();
    return 0;
}
