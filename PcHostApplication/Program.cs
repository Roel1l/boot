using MQTTnet;

var factory = new MqttClientFactory();
var mqttClient = factory.CreateMqttClient();

var options = new MqttClientOptionsBuilder()
    .WithTcpServer("test.mosquitto.org", 1883)
    .WithClientId("pchost_publisher_" + Guid.NewGuid())
    .Build();

await mqttClient.ConnectAsync(options);
Console.WriteLine("Connected to MQTT broker.");

while (true)
{
    Console.Write("Enter message to send (or 'exit'): ");
    var input = Console.ReadLine();
    if (input == null || input.ToLower() == "exit")
        break;

    var message = new MqttApplicationMessageBuilder()
        .WithTopic("wii/test")
        .WithPayload(input)
        .Build();

    await mqttClient.PublishAsync(message);
    Console.WriteLine("Message sent.");
}

await mqttClient.DisconnectAsync();
Console.WriteLine("Disconnected.");
