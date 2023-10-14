// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
// /*
//   ota.h
//   Header file that contains the HTML code for the OTA update page.
// */
// /////////////////////////////////////////////////////////////////

const char* OTA_PAGE = R"rawString(
<!DOCTYPE html>
<html>
<head>
    <title>ESP OTA Update</title>
    <script>
        function rebootESP() {
            fetch('/reboot')
            .then(response => response.text())
            .then(data => {
                alert(data); // Alert the response (e.g., "Rebooting ESP...")
                // You can add any additional client-side logic here, if needed
            });
        }
    </script>
</head>
<body>
    <h1>Welcome to the OTA Updater</h1>
    <h2><p>Here, you can update the firmware of your device "Over-the-Air".</p></h2>
    <p>Follow the steps below:</p>
    <ol>
        <li>Navigate to the OTA Loader page. Link Below.</li>
        <li>Make sure to select the OTA MODE as Firmware.</li>
        <li>Select .bin file. <b>NOTE: Once you select the file, the file transmission starts automatically.</b></li>
        <li>Once file is loaded, the system will reset automatically.</li>
        <li><b>Don't want to update?</b> Just restart the system by clicking the Reboot ESP button.</li>
    </ol>
    <p><a href="/update">Go to OTA Update</a></p>
    <button onclick="rebootESP()">Reboot ESP</button>
</body>
</html>
)rawString";
