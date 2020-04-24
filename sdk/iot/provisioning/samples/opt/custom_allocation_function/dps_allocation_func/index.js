module.exports = function (context, req) {
    var data = req.body;
    context.log('Input data: ' + JSON.stringify(data));
   
    var result = "";
   
    if ('linkedHubs' in data)
    {
    var randomIndex = getRandomIndex(data.linkedHubs.length);
    result = data.linkedHubs[randomIndex];
    context.log('Selected IoT Hub: ' + result);
    }
   
    var payload = null;
    //if (((((data || {}).enrollmentGroup || {}).initialTwin || {}).tags || {}).returnData)
    //{
    // returnData = data.deviceRuntimeContext.data; // returning input data
    //}
   
    //payload = {specialUrl: 'github.com/foo', inputData: data.deviceRuntimeContext.data}; //500
    payload = {hello: "world", arr: [1,2,3,4,5,6], num:123};    // OK, payload
    //payload = {hello: "world"}    // OK, payload
    //payload = 1; // Fail 400, no payload
    //payload = "123"; // Fail 400, no payload
    //payload = null; // OK, no payload


    var initialTwin = {
    "tags": {
    "twinReturnedFromWebhook": true
    }
    };
   
    context.res = {
    body: {
    "iotHubHostName": result,
    "initialTwin": initialTwin,
    "payload": payload
    }
    }
       context.log('output result.body: ' + JSON.stringify(context.res.body));
    context.done();
   }
   
   function getRandomIndex(maximum) {
    return Math.floor(Math.random() * Math.floor(maximum));
}