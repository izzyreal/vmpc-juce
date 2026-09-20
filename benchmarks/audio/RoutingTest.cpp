#include "engine/audio/server/RealTimeAudioServer.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace mpc::engine::audio::server;
namespace
{
    void require(bool condition)
    {
        if (!condition)
        {
            throw std::runtime_error("Audio server channel routing regression");
        }
    }
    struct Client : AudioClient
    {
        std::vector<std::shared_ptr<IOAudioProcess>> inputs, outputs;
        void work(int frames) override
        {
            for (int channel = 0; channel < 12; ++channel)
            {
                for (int frame = 0; frame < frames; ++frame)
                {
                    require(inputs[channel / 2]
                                ->localBuffer[frame * 2 + channel % 2] ==
                            float(100 * channel + frame));
                    outputs[channel / 2]->localBuffer[frame * 2 + channel % 2] =
                        float(1000 + 100 * channel + frame);
                }
            }
        }
    };
} // namespace
int main()
{
    try
    {
        for (int frames : {1, 64, 256, 1024})
        {
            RealTimeAudioServer server;
            auto client = std::make_shared<Client>();
            for (int i = 0; i < 6; ++i)
            {
                client->inputs.push_back(
                    server.openAudioInput(std::to_string(i)));
                client->outputs.push_back(
                    server.openAudioOutput(std::to_string(i)));
            }
            server.resizeBuffers(frames);
            server.setClient(client);
            server.start();
            std::array<std::vector<float>, 12> input, output;
            std::array<const float *, 12> in{};
            std::array<float *, 12> out{};
            std::vector<int8_t> channels, hosts;
            for (int channel = 0; channel < 12; ++channel)
            {
                const int host = 11 - channel;
                input[host].resize(frames);
                output[host].assign(frames, -1.f);
                for (int frame = 0; frame < frames; ++frame)
                {
                    input[host][frame] = float(100 * channel + frame);
                }
                in[host] = input[host].data();
                out[host] = output[host].data();
                channels.push_back(int8_t(channel));
                hosts.push_back(int8_t(host));
            }
            // Repeat to exercise both initial and unchanged input mappings.
            for (int repetition = 0; repetition < 2; ++repetition)
            {
                server.work(in.data(), out.data(), frames, channels, channels,
                            hosts, hosts);
                for (int channel = 0; channel < 12; ++channel)
                {
                    for (int frame = 0; frame < frames; ++frame)
                    {
                        require(output[11 - channel][frame] ==
                                float(1000 + 100 * channel + frame));
                    }
                }
            }
        }
        std::cout << "Audio server input/output routing tests passed.\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
