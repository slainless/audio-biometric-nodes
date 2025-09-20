namespace MqttHeader {
constexpr char WILL[] = "ded";
constexpr char VERIFY[] = "verify";
} // namespace MqttHeader

namespace MqttMessageType {
constexpr char MESSAGE[] = "msg";

constexpr char FRAGMENT_HEADER[] = "head";
constexpr char FRAGMENT_BODY[] = "frag";
constexpr char FRAGMENT_TRAILER[] = "end";
} // namespace MqttMessageType

namespace MqttTopic {
constexpr char RECORDER[] = "audio_biometric/slainless/device/recorder";
}