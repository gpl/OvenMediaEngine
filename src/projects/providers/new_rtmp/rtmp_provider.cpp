//==============================================================================
//
//  RtmpProvider
//
//  Created by Getroot
//  Copyright (c) 2018 AirenSoft. All rights reserved.
//
//==============================================================================
#include <config/config.h>
#include <unistd.h>
#include <iostream>

#include "rtmp_provider.h"
#include "rtmp_application.h"
#include "rtmp_stream.h"

#include <modules/physical_port/physical_port_manager.h>
#include <base/info/media_extradata.h>

#define OV_LOG_TAG "RtmpProvider"

std::shared_ptr<RtmpProvider> RtmpProvider::Create(const cfg::Server &server_config, const std::shared_ptr<MediaRouteInterface> &router)
{
	auto provider = std::make_shared<RtmpProvider>(server_config, router);
	if (!provider->Start())
	{
		logte("An error occurred while creating RtmpProvider");
		return nullptr;
	}
	return provider;
}

RtmpProvider::RtmpProvider(const cfg::Server &server_config, const std::shared_ptr<MediaRouteInterface> &router)
	: PushProvider(server_config, router)
{
	logtd("Created Rtmp Provider module.");
}

RtmpProvider::~RtmpProvider()
{
	logti("Terminated Rtmp Provider module.");
}

bool RtmpProvider::Start()
{
	if (_physical_port != nullptr)
	{
		logtw("RTMP server is already running");
		return false;
	}

	auto server = GetServerConfig();
	auto rtmp_address = ov::SocketAddress(server.GetIp(), static_cast<uint16_t>(server.GetBind().GetProviders().GetRtmpPort()));

	_physical_port = PhysicalPortManager::Instance()->CreatePort(ov::SocketType::Tcp, rtmp_address);
	if (_physical_port == nullptr)
	{
		logte("Could not initialize phyiscal port for RTMP server: %s", rtmp_address.ToString().CStr());
		return false;
	}

	_physical_port->AddObserver(this);

	return Provider::Start();
}

bool RtmpProvider::Stop()
{
	_physical_port->RemoveObserver(this);
	PhysicalPortManager::Instance()->DeletePort(_physical_port);

	_physical_port = nullptr;

	return Provider::Stop();
}

std::shared_ptr<pvd::Application> RtmpProvider::OnCreateProviderApplication(const info::Application &application_info)
{
	return RtmpApplication::Create(pvd::Provider::GetSharedPtrAs<pvd::Provider>(), application_info);
}

bool RtmpProvider::OnDeleteProviderApplication(const std::shared_ptr<pvd::Application> &application)
{
	// delete all client of streams from the physical port

	return true;
}

void RtmpProvider::OnConnected(const std::shared_ptr<ov::Socket> &remote)
{
	PushProvider::OnConnected(remote->GetId());
}

void RtmpProvider::OnDataReceived(const std::shared_ptr<ov::Socket> &remote,
					const ov::SocketAddress &address,
					const std::shared_ptr<const ov::Data> &data)
{
	PushProvider::OnDataReceived(remote->GetId(), data);
}

void RtmpProvider::OnDisconnected(const std::shared_ptr<ov::Socket> &remote,
					PhysicalPortDisconnectReason reason,
					const std::shared_ptr<const ov::Error> &error)
{
	PushProvider::OnDisconnected(remote->GetId(), error);
}

std::shared_ptr<pvd::PushProviderStream> RtmpProvider::CreateStreamMold()
{
	return RtmpStream::Create(GetSharedPtrAs<pvd::PushProvider>());
}