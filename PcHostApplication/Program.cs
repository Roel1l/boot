using MQTTnet;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;

var builder = WebApplication.CreateBuilder(args);
var app = builder.Build();

var factory = new MqttClientFactory();
var mqttClient = factory.CreateMqttClient();

var options = new MqttClientOptionsBuilder()
    .WithTcpServer("test.mosquitto.org", 1883)
    .WithClientId("pchost_publisher_" + Guid.NewGuid())
    .Build();

await mqttClient.ConnectAsync(options);
Console.WriteLine("Connected to MQTT broker.");

app.MapGet("/send", async (HttpRequest request) =>
{
    var messageText = request.Query["message"].ToString();
    if (string.IsNullOrWhiteSpace(messageText))
        return Results.BadRequest("Missing 'message' query parameter.");

    var message = new MqttApplicationMessageBuilder()
        .WithTopic("wii/test")
        .WithPayload(messageText)
        .Build();

    await mqttClient.PublishAsync(message);
    return Results.Ok($"Message '{messageText}' sent.");
});

// Start the web server and listen on all network interfaces for port 5000
_ = app.RunAsync("http://0.0.0.0:5000");

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
