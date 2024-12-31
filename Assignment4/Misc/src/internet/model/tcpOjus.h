#ifndef TCP_OJUS_H
#define TCP_OJUS_H

#include "tcp-congestion-ops.h"
#include "ns3/nstime.h"

namespace ns3 {

class TcpOjus : public TcpCongestionOps
{
public:
  static TypeId GetTypeId (void);
  TcpOjus ();
  virtual ~TcpOjus ();

  virtual std::string GetName () const override;
  virtual Ptr<TcpCongestionOps> Fork () override;

private:
  void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
  void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  void HystartReset (Ptr<const TcpSocketState> tcb);
  void HystartUpdate (Ptr<TcpSocketState> tcb, const Time& delay);
  void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
  void CongestionStateSet(Ptr<TcpSocketState> tcb,const TcpSocketState::TcpCongState_t newState) override;
  Time HystartDelayThresh (const Time& t) const;
  uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
  SequenceNumber32 m_endSeq;

  // Parameters and variables for hystart
  bool m_hystart;                              // Hystart toggle
  bool m_found;                                // Flag for exiting Slow Start
  uint32_t m_hystartLowWindow;                 // Min cwnd to apply Hystart
  uint8_t m_hystartMinSamples;                 // Min delay samples for Hystart
  uint8_t m_sampleCnt;                         // Sample count for delay measurements
  Time m_hystartAckDelta;                      // ACK spacing threshold
  Time m_hystartDelayMin;                      // Min delay threshold for Hystart
  Time m_hystartDelayMax;                      // Max delay threshold for Hystart
  Time m_delayMin;
  Time m_roundStart;                           // Start of Hystart round
  Time m_lastAck;                              // Time of last ACK
  Time m_currRtt;                              // Current RTT for Hystart
};

} 

#endif 