// tcpOjus.cc
#include "tcpOjus.h"
#include "tcp-congestion-ops.h"
#include "tcp-socket-base.h"
#include "ns3/log.h"
#include "ns3/tcp-socket-state.h"
// #include "ns3/sequence-number.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpOjus");
NS_OBJECT_ENSURE_REGISTERED (TcpOjus);

TypeId
TcpOjus::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpOjus")
    .SetParent<TcpCongestionOps> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpOjus> ()
    .AddAttribute ("HyStart",
                   "Enable (true) or disable (false) hybrid slow start algorithm",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpOjus::m_hystart),
                   MakeBooleanChecker ())
    .AddAttribute ("HyStartLowWindow",
                   "Lower bound cwnd for hybrid slow start (segments)",
                   UintegerValue (16),
                   MakeUintegerAccessor (&TcpOjus::m_hystartLowWindow),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("HyStartMinSamples",
                   "Number of delay samples for detecting delay increase",
                   UintegerValue (32),
                   MakeUintegerAccessor (&TcpOjus::m_hystartMinSamples),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("HyStartAckDelta",
                   "Spacing between ACKs indicating packet train",
                   TimeValue (MilliSeconds (2)),
                   MakeTimeAccessor (&TcpOjus::m_hystartAckDelta),
                   MakeTimeChecker ())
    .AddAttribute ("HyStartDelayMin",
                   "Minimum time for Hystart algorithm",
                   TimeValue (MilliSeconds (8)),
                   MakeTimeAccessor (&TcpOjus::m_hystartDelayMin),
                   MakeTimeChecker ())
    .AddAttribute ("HyStartDelayMax",
                   "Maximum time for Hystart algorithm",
                   TimeValue (MilliSeconds (1024)),
                   MakeTimeAccessor (&TcpOjus::m_hystartDelayMax),
                   MakeTimeChecker ());
  return tid;
}

TcpOjus::TcpOjus ()
  : TcpCongestionOps (),
    m_hystart (true),
    m_hystartLowWindow (32),
    m_hystartMinSamples (32),
    m_delayMin(Time::Min()),
    m_hystartAckDelta (MilliSeconds (2)),
    m_hystartDelayMin (MilliSeconds (8)),
    m_hystartDelayMax (MilliSeconds (2048)),
    m_roundStart (Time::Min ()),
    m_lastAck (Time::Min ()),
    m_currRtt (Time::Min ()),
    m_sampleCnt (0),
    m_found (false)
{
}

TcpOjus::~TcpOjus () {}

std::string
TcpOjus::GetName () const
{
  return "TcpOjus";
}

Ptr<TcpCongestionOps>
TcpOjus::Fork ()
{
  return CreateObject<TcpOjus> ();
}

void
TcpOjus::HystartReset (Ptr<const TcpSocketState> tcb)
{
  m_roundStart = m_lastAck = Simulator::Now ();
  m_endSeq = tcb->m_highTxMark;
  m_currRtt = Time::Min ();
  m_sampleCnt = 0;
  m_found = false;
}


Time
TcpOjus::HystartDelayThresh (const Time& t) const
{
  Time ret = t;
  if (t > m_hystartDelayMax)
  {
    ret = m_hystartDelayMax;
  }
  else if (t < m_hystartDelayMin)
  {
    ret = m_hystartDelayMin;
  }
  return ret;
}

void
TcpOjus::HystartUpdate (Ptr<TcpSocketState> tcb, const Time& delay)
{
  if (!m_found)
  {
    Time now = Simulator::Now ();
    if ((now - m_lastAck) <= m_hystartAckDelta)
    {
      m_lastAck = now;
      if ((now - m_roundStart) > m_delayMin)
      {
        m_found = true;
      }
    }

    if (m_sampleCnt < m_hystartMinSamples)
    {
      m_currRtt = std::min (m_currRtt, delay);
      ++m_sampleCnt;
    }
    else if (m_currRtt > m_delayMin + HystartDelayThresh (m_delayMin))
    {
      m_found = true;
    }

    if (m_found)
    {
      tcb->m_ssThresh = tcb->m_cWnd;
      NS_LOG_DEBUG ("Exiting Slow Start due to Hystart");
    }
  }
}


void
TcpOjus::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);

    /* Discard delay samples right after fast recovery */
    // if (m_epochStart != Time::Min() && (Simulator::Now() - m_epochStart) < m_cubicDelta)
    // {
    //     return;
    // }

    /* first time call or link delay decreases */
    if (m_delayMin == Time::Min() || m_delayMin > rtt)
    {
        m_delayMin = rtt;
    }

    /* hystart triggers when cwnd is larger than some threshold */
    if (m_hystart && tcb->m_cWnd <= tcb->m_ssThresh &&
        tcb->m_cWnd >= m_hystartLowWindow * tcb->m_segmentSize)
    {
        HystartUpdate(tcb, rtt);
    }
}

void
TcpOjus::CongestionStateSet(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{
    NS_LOG_FUNCTION(this << tcb << newState);

    if (newState == TcpSocketState::CA_LOSS)
    {
        m_found = false;
        HystartReset(tcb);
        m_delayMin = Time::Min();
    }
}

uint32_t
TcpOjus::GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    // Sets the slow start threshold to half the current bytes in flight
    return std::max(3 * tcb->m_segmentSize, bytesInFlight / 2 );
}



void
TcpOjus::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  if (tcb->m_cWnd < tcb->m_ssThresh)
  {
    if (m_hystart && tcb->m_lastAckedSeq > m_endSeq)
    {
      HystartReset (tcb);
    }
    tcb->m_cWnd += 1.2 *segmentsAcked * tcb->m_segmentSize;
    segmentsAcked = 0;
  }
  else
  {
    CongestionAvoidance (tcb, segmentsAcked);
  }
}


void TcpOjus::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) {
  NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (segmentsAcked > 0)
    {
        double adder =
            static_cast<double>(tcb->m_segmentSize * tcb->m_segmentSize) / tcb->m_cWnd.Get();
        adder = std::max(1.0, adder);
        tcb->m_cWnd += static_cast<uint32_t>(adder);
        NS_LOG_INFO("In CongAvoid, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                     << tcb->m_ssThresh);
    }
}

} // namespace ns3
