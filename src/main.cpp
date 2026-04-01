#include <cstdio>
#include <cstring>
#include <jack/jack.h>
#include <string>
#include <unistd.h>
#include <vector>

jack_client_t *client = nullptr;
jack_port_t *in_port = nullptr;
jack_port_t *out_port_l = nullptr;
jack_port_t *out_port_r = nullptr;

int process(jack_nframes_t nframes, void *) {
    // jack_port_get_buffer returns a void* to the audio buffer for that port, and we cast it to a
    // float because jack audio is in 32 bit float samples. auto* lets the compiler infer the
    // pointer type from the cast. JACK itself manages the memory for these buffers, and the input
    // ports are filled with audio data from the previous cycle and you write to the output buffer
    // in the callback, which is then sent to the hardware when the callback returns.
    auto *in = (float *)jack_port_get_buffer(in_port, nframes);
    auto *out_l = (float *)jack_port_get_buffer(out_port_l, nframes);
    auto *out_r = (float *)jack_port_get_buffer(out_port_r, nframes);

    // std::memcpy copies raw bytes from a source to a destination. In the third param,
    // sizeof(float) is the size of a float in bytes which is 4, and nframes is the buffer size
    // (256). This means that each buffer contains 1024 bytes of data, and we have to specify to
    // memcpy that this is the exact amount of data we want to copy.
    std::memcpy(out_l, in, sizeof(float) * nframes);
    std::memcpy(out_r, in, sizeof(float) * nframes);

    return 0;
}

// You pass in the pointer to the jack client, and jack style flags.
std::vector<std::string> get_port_list(jack_client_t *client, unsigned long flags) {

    // std::vector<std::string> is a C++ resizable array of strings.
    std::vector<std::string> result;

    // JACK get ports returns a null terminated array of C strings, meaning an array with the final
    // element being nullptr which signifies the end.
    const char **ports = jack_get_ports(client, nullptr, JACK_DEFAULT_AUDIO_TYPE, flags);

    // If the above fails, result will just contain nullptr, which is then returned to signify a
    // failure.
    if (!ports)
        return result;

    // Walks through the jack array until it finds nullptr, and appends each array item to our C++
    // array of strings, result.
    for (int i = 0; ports[i] != nullptr; i++)
        result.push_back(ports[i]);

    // We have to free jack's array now that we are done with it. I still don't understand this, but
    // it's necessary.
    jack_free(ports);
    return result;
}

// gotta figure out what the ins of this function mean still
int prompt_choice(const std::string &prompt, const std::vector<std::string> &options) {
    // printf expects a const char*, not a std::string. c_str gives a pointer to the character
    // buffer for the std::string that normal C functions can read.
    printf("\n%s\n", prompt.c_str());

    // size_t is an unsized integer type used for sizes, and is what options.size returns. %zu tells
    // printf it is size_t.
    for (size_t i = 0; i < options.size(); i++)
        printf("  [%zu] %s\n", i, options[i].c_str());

    // Making choice -1 initially makes the while run at least once, as shown earlier options.size
    // returns type size_t which is unsigned, so we cast it to an int to compare without causing
    // issues between a signed int and an unsigned size_t.
    int choice = -1;
    while (choice < 0 || choice >= (int)options.size()) {
        // scanf with the %d format attempts to parse an integer from stdin. &choice is the address
        // of choice, which scanf will then write into. It returns the number of items successfully
        // parsed, so anything other than 1 means incorrect input. This causes choice to be set to
        // -1 and input to be prompted again.
        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
            }
            choice = -1;
        }
    }
    return choice;
}

int main() {
    // This is a status variable that jack can write to if something goes wrong. The address of this
    // variable is passed to jack when the client is registered.
    jack_status_t status;

    // Registers this program with the jack server.
    client = jack_client_open("cpp-fxprocessor", JackNullOption, &status);
    // If the client failed to exist the program exits with code 1 and prints could not connect to
    // jack to stderr.
    if (!client) {
        fprintf(stderr, "Could not connect to JACK\n");
        return 1;
    }

    printf("Sample rate: %d\n", jack_get_sample_rate(client));
    printf("Buffer size: %d\n", jack_get_buffer_size(client));

    // Capture ports are physical outputs.
    auto capture_ports = get_port_list(client, JackPortIsPhysical | JackPortIsOutput);
    // Playback ports are physical inputs.
    auto playback_ports = get_port_list(client, JackPortIsPhysical | JackPortIsInput);

    if (capture_ports.empty()) {
        fprintf(stderr, "No capture ports found.\n");
        jack_client_close(client);
        return 1;
    }

    if (playback_ports.size() < 2) {
        fprintf(stderr, "Needs at least 2 playback ports.\n");
        jack_client_close(client);
        return 1;
    }

    // Prompts the user and asks what ports they want to use out of the grabbed list.
    int in_choice = prompt_choice("Select input port:", capture_ports);
    int out_choice_l = prompt_choice("Select left output port:", playback_ports);
    int out_choice_r = prompt_choice("Select right output port:", playback_ports);

    // This sets the process function as the callback. The third param is the void pointer on the
    // input of the process function. You point it at your own data for the callback if that's
    // something you need.
    jack_set_process_callback(client, process, nullptr);

    // This creates the jack ports for this client.
    in_port = jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    out_port_l =
        jack_port_register(client, "output_l", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    out_port_r =
        jack_port_register(client, "output_r", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    // This is when jack starts calling the callback. Everything before this is setup.
    jack_activate(client);

    // Connect client ports to physical device ports based on user selections.
    jack_connect(client, capture_ports[in_choice].c_str(), jack_port_name(in_port));
    jack_connect(client, jack_port_name(out_port_l), playback_ports[out_choice_l].c_str());
    jack_connect(client, jack_port_name(out_port_r), playback_ports[out_choice_r].c_str());

    printf("\nConnected:\n");
    printf("  %s -> cpp-fxprocessor:input\n", capture_ports[in_choice].c_str());
    printf("  cpp-fxprocessor:output_l -> %s\n", playback_ports[out_choice_l].c_str());
    printf("  cpp-fxprocessor:output_r -> %s\n", playback_ports[out_choice_r].c_str());
    printf("\nRunning, Press Ctrl+C to break\n");

    // pauses here before continuing until a signal arrives, like sigint.
    pause();

    // cleanly disconnects the jack client from jack before closing.
    jack_client_close(client);
    return 0;
}