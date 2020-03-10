/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#include "ns3/custom-periodic-sender.h"
#include "ns3/pointer.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/lora-net-device.h"

#include <stdio.h>  /* defines FILENAME_MAX */
#include <unistd.h>
#define GetCurrentDir getcwd
#include <fstream>
#include <iostream>

std::string GetCurrentWorkingDir( void ) {
  char buff[FILENAME_MAX];
  GetCurrentDir( buff, FILENAME_MAX );
  std::string current_working_dir(buff);
  return current_working_dir;
}

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("CustomPeriodicSender");

NS_OBJECT_ENSURE_REGISTERED (CustomPeriodicSender);

TypeId
CustomPeriodicSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CustomPeriodicSender")
    .SetParent<Application> ()
    .AddConstructor<CustomPeriodicSender> ()
    .SetGroupName ("lorawan")
    .AddAttribute ("Interval", "The interval between packet sends of this app",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CustomPeriodicSender::GetInterval,
                                     &CustomPeriodicSender::SetInterval),
                   MakeTimeChecker ());
  // .AddAttribute ("PacketSizeRandomVariable", "The random variable that determines the shape of the packet size, in bytes",
  //                StringValue ("ns3::UniformRandomVariable[Min=0,Max=10]"),
  //                MakePointerAccessor (&CustomPeriodicSender::m_pktSizeRV),
  //                MakePointerChecker <RandomVariableStream>());
  return tid;
}

CustomPeriodicSender::CustomPeriodicSender ()
  : m_interval (Seconds (10)),
  m_initialDelay (Seconds (1)),
  m_basePktSize (10),
  m_pktSizeRV (0)

{
  NS_LOG_FUNCTION_NOARGS ();
}

CustomPeriodicSender::~CustomPeriodicSender ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
CustomPeriodicSender::SetInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_interval = interval;
}

Time
CustomPeriodicSender::GetInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_interval;
}

void
CustomPeriodicSender::SetInitialDelay (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_initialDelay = delay;
}


void
CustomPeriodicSender::SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv)
{
  m_pktSizeRV = rv;
}


void
CustomPeriodicSender::SetPacketSize (uint8_t size)
{
  m_basePktSize = size;
}


void
CustomPeriodicSender::SendPacket (void)
{
  NS_LOG_FUNCTION (this);

  // Create and send a new packet
  Ptr<Packet> packet;
  if (m_pktSizeRV)
    {
      int randomsize = m_pktSizeRV->GetInteger ();
      packet = Create<Packet> (m_basePktSize + randomsize);
    }
  else
    {
      packet = Create<Packet> (m_basePktSize);
    }
  m_mac->Send (packet);

  uint32_t node_id = m_node->GetId();
  std::string buf = this->ReadOffset(node_id);
  NS_LOG_DEBUG ("Offset value: " << buf);
  if (buf != "0")
      this->InitOffset(node_id);

  Time offset = Seconds(std::stod(buf));

  NS_LOG_DEBUG ("Add offset of " << offset.GetSeconds ());

  // Schedule the next SendPacket event
  m_sendEvent = Simulator::Schedule (offset + m_interval, &CustomPeriodicSender::SendPacket,
                                     this);

  NS_LOG_DEBUG ("Sent a packet of size " << packet->GetSize ());
}

void
CustomPeriodicSender::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  // Make sure we have a MAC layer
  if (m_mac == 0)
    {
      // Assumes there's only one device
      Ptr<LoraNetDevice> loraNetDevice = m_node->GetDevice (0)->GetObject<LoraNetDevice> ();

      m_mac = loraNetDevice->GetMac ();
      NS_ASSERT (m_mac != 0);
    }

  // this->InitOffset();

  // Schedule the next SendPacket event
  Simulator::Cancel (m_sendEvent);
  NS_LOG_DEBUG ("Starting up application with a first event with a " <<
                m_initialDelay.GetSeconds () << " seconds delay");
  m_sendEvent = Simulator::Schedule (m_initialDelay,
                                     &CustomPeriodicSender::SendPacket, this);
  NS_LOG_DEBUG ("Event Id: " << m_sendEvent.GetUid ());
}

void
CustomPeriodicSender::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);
}

void
CustomPeriodicSender::InitOffset (uint32_t node_id)
{
  // offsetファイルの作成
  NS_LOG_DEBUG ("Offset file initiating ...");
  std::ofstream out("offset/" + std::to_string(node_id) + ".txt", std::ios::out);
  // NS_LOG_DEBUG ("Current path: " << GetCurrentWorkingDir());
  char init_val = '0';
  //入力された内容をファイルに書き込む
  out << init_val << std::endl;
}

std::string
CustomPeriodicSender::ReadOffset (uint32_t node_id)
{
  // offsetファイルの読み取り
  NS_LOG_DEBUG ("Reading offset file");
  std::ifstream ifs("offset/" + std::to_string(node_id) + ".txt");
  std::string res;
  //ファイルからstringにデータ格納
  ifs >> res;
  return res;
}

}
}
