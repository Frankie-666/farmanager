/*
PluginSynchro.cpp
������������� ��� ��������.
*/
/*
Copyright � 2009 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#pragma hdrstop

#include "PluginSynchro.hpp"
#include "plclass.hpp"
#include "elevation.hpp"
#include "FarGuid.hpp"
#include "ctrlobj.hpp"
#include "plugins.hpp"

PluginSynchro& PluginSynchroManager()
{
	static PluginSynchro ps;
	return ps;
}

struct PluginSynchro::SynchroData
{
	bool Plugin;
	GUID PluginId;
	void* Param;
};

PluginSynchro::PluginSynchro():
	m_suppressions()
{
}

PluginSynchro::~PluginSynchro()
{
}

void PluginSynchro::Synchro(bool Plugin, const GUID& PluginId,void* Param)
{
	SCOPED_ACTION(CriticalSectionLock)(CS);
	SynchroData item;
	item.Plugin=Plugin;
	item.PluginId=PluginId;
	item.Param=Param;
	Data.emplace_back(item);
}

bool PluginSynchro::Process()
{
	bool res=false;
	bool process=false; bool plugin=false; GUID PluginId=FarGuid; void* param=nullptr;
	{
		SCOPED_ACTION(CriticalSectionLock)(CS);

		if (!m_suppressions && !Data.empty())
		{
			auto ItemIterator = Data.begin();
			process=true;
			plugin=ItemIterator->Plugin;
			PluginId=ItemIterator->PluginId;
			param=ItemIterator->Param;
			Data.pop_front();
		}
	}

	if (process)
	{
		if(plugin)
		{
			Plugin* pPlugin = Global->CtrlObject? Global->CtrlObject->Plugins->FindPlugin(PluginId) : nullptr;

			if (pPlugin)
			{
				ProcessSynchroEventInfo Info = {sizeof(Info)};
				Info.Event = SE_COMMONSYNCHRO;
				Info.Param = param;

				pPlugin->ProcessSynchroEvent(&Info);
				res=true;
			}
		}
		else
		{
			ElevationApproveDlgSync(param);
			res=true;
		}
	}


	return res;
}

PluginSynchro::suppress::suppress():
	m_owner(PluginSynchroManager())
{
	InterlockedIncrement(&m_owner.m_suppressions);
}

PluginSynchro::suppress::~suppress()
{
	InterlockedDecrement(&m_owner.m_suppressions);
}