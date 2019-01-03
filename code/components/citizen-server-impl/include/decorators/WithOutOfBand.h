#pragma once

namespace fx
{
	namespace ServerDecorators
	{
		template<typename... TOOB>
		const fwRefContainer<fx::GameServer>& WithOutOfBand(const fwRefContainer<fx::GameServer>& server)
		{
			// this is static because GCC seems to get really confused when things are wrapped in a lambda
			static std::map<std::string, std::function<void(const fwRefContainer<fx::GameServer>& server, const net::PeerAddress& from, const std::string_view& data)>, std::less<>> processors;

			pass{ ([&]()
			{
				auto oob = TOOB();
				processors.insert({ oob.GetName(), std::bind(&std::remove_reference_t<decltype(oob)>::Process, &oob, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) });
			}(), 1)... };

			server->AddRawInterceptor([server](const uint8_t* receivedData, size_t receivedDataLength, const net::PeerAddress& receivedAddress)
			{
				// workaround a VS15.7 compiler bug that drops `const` qualifier in the std::function
				fwRefContainer<fx::GameServer> tempServer = server;

				if (receivedDataLength >= 4 && *reinterpret_cast<const int*>(receivedData) == -1)
				{
					auto begin = receivedData + 4;
					auto len = receivedDataLength - 4;

					std::string_view sv(reinterpret_cast<const char*>(begin), len);
					auto from = receivedAddress;

					auto key = sv.substr(0, sv.find_first_of(" \n"));
					auto data = sv.substr(sv.find_first_of(" \n") + 1);

					auto it = processors.find(key);

					if (it != processors.end())
					{
						it->second(tempServer, from, data);
					}

					return true;
				}

				return false;
			});

			return server;
		}
	}
}
