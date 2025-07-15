using MQTTnet;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.DependencyInjection;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

var app = builder.Build();

app.UseSwagger();
app.UseSwaggerUI();

var factory = new MqttClientFactory();
var mqttClient = factory.CreateMqttClient();

var options = new MqttClientOptionsBuilder()
    .WithTcpServer("test.mosquitto.org", 1883)
    .WithClientId("pchost_publisher_" + Guid.NewGuid())
    .Build();

await mqttClient.ConnectAsync(options);
Console.WriteLine("Connected to MQTT broker.");

app.MapGet("/", context => {
    context.Response.Redirect("/swagger");
    return Task.CompletedTask;
});

app.MapGet("/messageWii", async (string message) =>
{
    if (string.IsNullOrWhiteSpace(message))
        return Results.BadRequest("Missing 'message' query parameter.");

    var mqttMessage = new MqttApplicationMessageBuilder()
        .WithTopic("wii/test")
        .WithPayload(message)
        .Build();

    await mqttClient.PublishAsync(mqttMessage);
    return Results.Ok($"Message '{message}' sent.");
});

// Start the web server and listen on all network interfaces for port 80
_ = app.RunAsync("http://0.0.0.0:80");

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
