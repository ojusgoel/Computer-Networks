/*create a dumbbell topology of 4 leaves. Create two TCP and two UDP flows between the left and
 * right nodes, with defined data rates and packet sizes and ensure the bottleneck link is 2 Mbps.
 * Enable tracing, generate PCAP files for packet capture, and output a NetAnim trace file for
 * visualization. Routing tables of the routers should be printed, and the simulation should run for
 * 10 seconds.*/

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/log.h"

using namespace ns3;

void
printStats(FlowMonitorHelper& flowmon_helper, bool perFlowInfo)
{
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon_helper.GetClassifier());
    std::string proto;
    Ptr<FlowMonitor> monitor = flowmon_helper.GetMonitor();
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    double totalTimeReceiving;
    uint64_t totalPacketsReceived, totalPacketsDropped, totalBytesReceived, totalPacketsTransmitted;

    totalBytesReceived = 0, totalPacketsDropped = 0, totalPacketsReceived = 0,
    totalTimeReceiving = 0, totalPacketsTransmitted = 0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::iterator flow = stats.begin();
         flow != stats.end();
         flow++)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow->first);
        switch (t.protocol)
        {
        case (6):
            proto = "TCP";
            break;
        case (17):
            proto = "UDP";
            break;
        default:
            exit(1);
        }
        totalBytesReceived += (double)flow->second.rxBytes * 8;
        totalTimeReceiving += flow->second.timeLastRxPacket.GetSeconds() -
                              flow->second.timeFirstTxPacket.GetSeconds();
        totalPacketsReceived += flow->second.rxPackets;
        totalPacketsDropped += flow->second.txPackets - flow->second.rxPackets;
        totalPacketsTransmitted += flow->second.txPackets;
        if (perFlowInfo)
        {
            std::cout << "FlowID: " << flow->first << " (" << proto << " " << t.sourceAddress
                      << " / " << t.sourcePort << " --> " << t.destinationAddress << " / "
                      << t.destinationPort << ")" << std::endl;
            std::cout << "  Throughput: "
                      << (((double)flow->second.rxBytes * 8) /
                          (flow->second.timeLastRxPacket.GetSeconds() -
                           flow->second.timeFirstTxPacket.GetSeconds()) /
                          1024 / 1024)
                      << " Mbps" << std::endl;
        }
    }

    std::cout << "Total packets transmitted: " << totalPacketsTransmitted << std::endl;
    std::cout << "Total packets received: " << totalPacketsReceived << std::endl;
    std::cout << "Total packets dropped: " << totalPacketsDropped << std::endl;
    std::cout << "Total throughput of System: "
              << (totalBytesReceived) / totalTimeReceiving / 1024 / 1024 << " Mbps " << std::endl;
    std::cout << " Packet Lost Ratio: "
              << totalPacketsDropped / (double)(totalPacketsReceived + totalPacketsDropped)
              << std::endl;
}

class MyApp : public Application
{
  public:
    MyApp();
    virtual ~MyApp();

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId(void);
    void Setup(Ptr<Socket> socket,
               Address address,
               uint32_t packetSize,
               uint32_t nPackets,
               DataRate dataRate);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void ScheduleTx(void);
    void SendPacket(void);

    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_nPackets;
    uint32_t m_packetsSent;
    bool m_running;
    DataRate m_dataRate;
    EventId m_sendEvent;
};

MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

MyApp::~MyApp()
{
    m_socket = 0;
}

/* static */
TypeId
MyApp::GetTypeId(void)
{
    static TypeId tid =
        TypeId("MyApp").SetParent<Application>().SetGroupName("Tutorial").AddConstructor<MyApp>();
    return tid;
}

void
MyApp::Setup(Ptr<Socket> socket,
             Address address,
             uint32_t packetSize,
             uint32_t nPackets,
             DataRate dataRate)
{
    m_socket = socket;
    m_peer = address;
    m_packetSize = packetSize;
    m_nPackets = nPackets;
    m_dataRate = dataRate;
}

void
MyApp::StartApplication(void)
{
    m_running = true;
    m_packetsSent = 0;
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        m_socket->Bind();
    }
    else
    {
        m_socket->Bind6();
    }
    m_socket->Connect(m_peer);
    SendPacket();
}

void
MyApp::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }
}

void
MyApp::SendPacket(void)
{
    Ptr<Packet> packet = Create<Packet>(m_packetSize);
    m_socket->Send(packet);

    if (++m_packetsSent < m_nPackets)
    {
        ScheduleTx();
    }
}

void
MyApp::ScheduleTx(void)
{
    if (m_running)
    {
        Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
        m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange(Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
    // NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldCwnd << "\t" << newCwnd
                         << std::endl;
}

static void
RttChange(Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldRtt.GetSeconds() << "\t"
                         << newRtt.GetSeconds() << std::endl;
}

static void
Ssthresh(Ptr<OutputStreamWrapper> stream, uint32_t oldSsthresh, uint32_t newSsthresh)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldSsthresh << "\t"
                         << newSsthresh << std::endl;

    //   NS_LOG_UNCOND ("Slow Start Threshold " << Simulator::Now ().GetSeconds () << "\t" <<
    //   oldSsthresh << "\t" << newSsthresh << std::endl);
}

void
CreateTcpFlowWithCcaAndTracing(Ptr<Node> srcNode,
                               Ptr<Node> destNode,
                               Ipv4Address destAddress,
                               uint16_t port,
                               std::string ccaTypeId,
                               std::string traceFilePrefix)
{
    // Print the ccaTypeId value
    //NS_LOG_UNCOND("CCA Type ID: " << ccaTypeId);

    // Configure CCA for the socket
    Config::Set("/NodeList/" + std::to_string(srcNode->GetId()) + "/$ns3::TcpL4Protocol/SocketType",
                StringValue(ccaTypeId));

    Address sinkAddress = InetSocketAddress(destAddress, port);

    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApps = packetSinkHelper.Install(destNode); // sinking on node 1

    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(20.));

    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(srcNode, TcpSocketFactory::GetTypeId());

    Ptr<MyApp> app = CreateObject<MyApp>();
    app->Setup(ns3TcpSocket, sinkAddress, 1040, 1000000, DataRate("100Mbps"));
    srcNode->AddApplication(app);
    app->SetStartTime(Seconds(1.));
    app->SetStopTime(Seconds(20.));

    AsciiTraceHelper asciiTraceHelper; // this is for the cwnd
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream(traceFilePrefix + ".cwnd");
    ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow",
                                             MakeBoundCallback(&CwndChange, stream));

    Ptr<OutputStreamWrapper> stream4 =
        asciiTraceHelper.CreateFileStream(traceFilePrefix + "_rtt.txt");
    ns3TcpSocket->TraceConnectWithoutContext("RTT", MakeBoundCallback(&RttChange, stream4));

    Ptr<OutputStreamWrapper> stream35 =
        asciiTraceHelper.CreateFileStream(traceFilePrefix + "-ssthresh.txt");
    ns3TcpSocket->TraceConnectWithoutContext("SlowStartThreshold",
                                             MakeBoundCallback(&Ssthresh, stream35));

}

double
CalculateJainsFairnessIndex(const std::vector<double>& throughputs)
{
    double sum = 0.0;
    double sumSquare = 0.0;

    for (double throughput : throughputs)
    {
        sum += throughput;
        sumSquare += throughput * throughput;
    }

    double n = throughputs.size();
    double ans = (sum * sum) / (n * sumSquare);
    return ans;
}

int
main(int argc, char* argv[])
{
    Time::SetResolution(Time::NS);

    uint32_t nflows = 2;

    CommandLine cmd;
    cmd.AddValue("nflows", "Data rate for the point-to-point link", nflows);
    cmd.Parse(argc, argv);

    // Create 4 nodes on the left side
    NodeContainer leftNodes;
    leftNodes.Create(nflows);

    // Create 2 routers in the middle
    NodeContainer routers;
    routers.Create(2);

    // Create 4 nodes on the right side
    NodeContainer rightNodes;
    rightNodes.Create(nflows);

    // Point-to-Point link parameters for leaf-to-router connections
    PointToPointHelper leafTorouter;
    leafTorouter.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    leafTorouter.SetChannelAttribute("Delay", StringValue("2ms"));

    // Install P2P links between the left nodes and the first router
    NetDeviceContainer leftDevices;
    for (uint32_t i = 0; i < leftNodes.GetN(); ++i)
    {
        NetDeviceContainer link = leafTorouter.Install(leftNodes.Get(i), routers.Get(0));
        leftDevices.Add(link);
    }

    // Set up the link between the two routers with a 2 Mbps data rate
    PointToPointHelper routerLink;
    routerLink.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    routerLink.SetChannelAttribute("Delay", StringValue("2ms"));

    // Install P2P link between the two routers
    NetDeviceContainer routerDevices = routerLink.Install(routers.Get(0), routers.Get(1));

    // Install P2P links between the right nodes and the second router
    NetDeviceContainer rightDevices;
    for (uint32_t i = 0; i < rightNodes.GetN(); ++i)
    {
        NetDeviceContainer link = leafTorouter.Install(rightNodes.Get(i), routers.Get(1));
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

    for (uint32_t i = 0; i < leftNodes.GetN(); ++i)
    {
        std::ostringstream subnet;
        subnet << "10.1." << i + 1 << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.0");
        leftInterfaces.Add(address.Assign(leftDevices.Get(i * 2)));
        address.Assign(leftDevices.Get(i * 2 + 1));
    }

    address.SetBase("10.1.60.0", "255.255.255.0");
    routerInterfaces = address.Assign(routerDevices);

    for (uint32_t i = 0; i < rightNodes.GetN(); ++i)
    {
        std::ostringstream subnet;
        subnet << "10.2." << i + 1 << ".0";
        address.SetBase(Ipv4Address(subnet.str().c_str()), "255.255.255.0");
        rightInterfaces.Add(address.Assign(rightDevices.Get(i * 2)));
        address.Assign(rightDevices.Get(i * 2 + 1));
    }

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Print routing table of the routers
    // Ptr<OutputStreamWrapper> routingStream1 = Create<OutputStreamWrapper>(&std::cout);
    // routers.Get(0)->GetObject<Ipv4>()->GetRoutingProtocol()->PrintRoutingTable(routingStream1);

    // Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper>(&std::cout);
    // routers.Get(1)->GetObject<Ipv4>()->GetRoutingProtocol()->PrintRoutingTable(routingStream2);

    // pointToPoint.BoundingBox (1, 1, 100, 100);

    // leafTorouter.EnablePcapAll("dumbbell-topology");

    uint32_t port = 121;

    for(uint i = 0; i < nflows; i++) {
        if(i < nflows/2)
            CreateTcpFlowWithCcaAndTracing(leftNodes.Get(i),
                                   rightNodes.Get(i),
                                   rightInterfaces.GetAddress(i),
                                   port + i,
                                   "ns3::TcpNewReno",
                                   "NewReno"+std::to_string(i));
        else
            CreateTcpFlowWithCcaAndTracing(leftNodes.Get(i),
                                   rightNodes.Get(i),
                                   rightInterfaces.GetAddress(i),
                                   port + i,
                                   "ns3::TcpOjus",
                                   "Ojus"+std::to_string(i));
    }

    Simulator::Stop(Seconds(15.0));

    // Create FlowMonitor
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> monitor = flowHelper.InstallAll();

    // Run the simulation
    Simulator::Run();
    printStats(flowHelper, true);

    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    std::vector<double> throughputs;

    for (auto& flow : stats)
    {
        double throughput = flow.second.rxBytes * 8.0 /
                            (flow.second.timeLastRxPacket.GetSeconds() -
                             flow.second.timeFirstTxPacket.GetSeconds()) /
                            1000000; // Mbps
        throughputs.push_back(throughput);
    }

    // Calculate Jain's Fairness Index
    double jainsFairnessIndex = CalculateJainsFairnessIndex(throughputs);
    std::cout << "Jain's Fairness Index: " << jainsFairnessIndex << std::endl;

    Simulator::Destroy();

    // Simulator::Run();
    // Simulator::Destroy();

    return 0;
}
