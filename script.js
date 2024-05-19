
var byteArray = [];


window.onload = function() {
    displayIndex();
    getDisplayDimensions()
}


async function getDisplayDimensions() {
    try {
        let widthResponse = await fetch('/api/v1/getDisplayWidth');
        if (!widthResponse.ok) {
            throw new Error('Network response was not ok');
        }
        let widthData = await widthResponse.json();

        let heightResponse = await fetch('/api/v1/getDisplayHeight');
        if (!heightResponse.ok) {
            throw new Error('Network response was not ok');
        }
        let heightData = await heightResponse.json();

        let displayWidth = widthData.displayWidth;
        let displayHeight = heightData.displayHeight;

        // Set the dimensions of the previewContainer
        let previewContainer = document.getElementById('previewContainer');
        previewContainer.style.width = displayWidth + 'px';
        previewContainer.style.height = displayHeight + 'px';

        // Display width and height
        document.getElementById('displayWidth').textContent = data.displayWidth;
        document.getElementById('displayHeight').textContent = data.displayHeight;

    } catch (error) {
        console.error('Error fetching display dimensions:', error);
    }
}




function displayIndex() {
    document.getElementById("htmlUpload").style.display = "none";
    document.getElementById("pngUpload").style.display = "block"; 
}

function displayPngUpload() {
    document.getElementById("htmlUpload").style.display = "none";
    document.getElementById("intro").style.display = "none";
    document.getElementById("pngUpload").style.display = "block"; 
}

function displayHtmlUpload() {
    document.getElementById("pngUpload").style.display = "none"
    document.getElementById("intro").style.display = "none";
    document.getElementById("htmlUpload").style.display = "block"; 
}




  function displayImage() {
  var fileInput = document.getElementById('fileInput');
  var file = fileInput.files[0];

  if (file) {
    var reader = new FileReader();

    reader.onload = function(e) {
      var imageContainer = document.getElementById('imageContainer');
      imageContainer.innerHTML = '<img src="' + e.target.result + '" alt="Uploaded Image" />';
    }

    reader.readAsDataURL(file);
  } else {
    alert('Bitte wählen Sie eine PNG-Datei aus.');

}
}

function convertAndUploadPng() {
    html2canvas(document.getElementById('imageContainer'), { scale: 1 }).then(canvas => {
    var context = canvas.getContext('2d');
    var imageData = context.getImageData(0, 0, canvas.width, canvas.height);
    var byteArray = convertImageDataToByteArray(imageData);
    console.log(byteArray);


            var url = '/api/v1/uploadbmp';

            // Konfiguration der Fetch-Anfrage
            var requestOptions = {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ data: byteArray })
            };

            // Senden der POST-Anfrage mit der Fetch-API
             fetch(url, requestOptions)
                 .then(function (response) {
                    // Überprüfen, ob die Anfrage erfolgreich war (Statuscode 200)
                     if (!response.ok) {
                     throw new Error('Fehler beim Senden der Daten. Statuscode: ' + response.status);
                    }
             return response.json(); // Rückgabe des JSON-Datenstroms der Antwort
            })
            .then(function (data) {
                console.log('Daten erfolgreich gesendet:', data);
            })
            .catch(function (error) {
                console.error('Fehler beim Senden der Daten:', error);
            });
        });

   
}
    
    function convertToHTML() {
        var inputText = document.getElementById("inputText").value;
        var preview = document.getElementById("preview");

        // Set the preview content
        preview.innerHTML = inputText;
    }


    function convertAndUpload() {
        html2canvas(document.querySelector("#preview"), { scale: 1 }).then(canvas => {
            // document.body.appendChild(canvas);
            // var img = canvas.toDataURL("image/png");
            // document.getElementById('previewImg').src = img;
            var img = canvas.toDataURL("image/png");
            document.getElementById('previewImg').src = img;
            var context = canvas.getContext("2d");
            // console.log(context.getImageData(0, 0, 800, 400));
            console.log(createImageBitmap(document.getElementById('previewImg'), 0, 0, 800, 480).data);
            var context = canvas.getContext("2d");
            var imageData = context.getImageData(0, 0, canvas.width, canvas.height);

            console.log(canvas.width, canvas.height);

            byteArray = convertImageDataToByteArray(imageData);
            console.log(byteArray);

            // console.log(context.getImageData(0, 0, 800, 400));
            // console.log(createImageBitmap(document.getElementById('previewImg'), 0, 0, 800, 480).data);

            var url = '/api/v1/uploadbmp';

            // Konfiguration der Fetch-Anfrage
            var requestOptions = {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ data: byteArray })
            };

            // Senden der POST-Anfrage mit der Fetch-API
             fetch(url, requestOptions)
                 .then(function (response) {
                    // Überprüfen, ob die Anfrage erfolgreich war (Statuscode 200)
                     if (!response.ok) {
                     throw new Error('Fehler beim Senden der Daten. Statuscode: ' + response.status);
                    }
             return response.json(); // Rückgabe des JSON-Datenstroms der Antwort
            })
            .then(function (data) {
                console.log('Daten erfolgreich gesendet:', data);
            })
            .catch(function (error) {
                console.error('Fehler beim Senden der Daten:', error);
            });
        });
    }

    // Helper function to find the closest color for a given pixel
    function findClosestColor(pixel) {
        // Extract RGB components from the pixel
        let r = pixel[0];
        let g = pixel[1];
        let b = pixel[2];
        let a = pixel[3];

        // Calculate the luminance
        let luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

        // Determine the closest color based on luminance and alpha
        if (a < 128) {
            return 0x00; // Transparent (Black)
        } else {
            if (luminance < 128) {
                // Dark colors: Black or Red
                if (r > g && r > b) {
                    return 0x03; // Red
                } else {
                    return 0x00; // Black
                }
            } else {
                // Light colors: White or Yellow
                if (r > g && r > b) {
                    return 0x02; // Yellow
                } else {
                    return 0x01; // White
                }
            }
        }
    }

    function convertImageDataToByteArray(imageData) {
        let data = imageData.data;
        let hexByteArray = [];

        for (let i = 0; i < data.length; i += 16) {
            let byte = 0x00;

            // Process 4 consecutive pixels at a time
            for (let j = 0; j < 16; j += 4) {
                let pixel = [data[i + j], data[i + j + 1], data[i + j + 2], data[i + j + 3]];

                // Find the closest color for the pixel and append it to the byte
                byte = (byte << 2) | findClosestColor(pixel);
            }

            // Push the byte to the hexByteArray
            hexByteArray.push(byte);
        }

        return hexByteArray;
    }



