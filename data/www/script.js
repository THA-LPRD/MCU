window.onload = function() {
    displayIndex();
    getDisplayDimensions()
}

  
let displayWidth, displayHeight;

   async function getDisplayDimensions() {
    try {
        let widthResponse = await fetch('/api/v1/GetDisplayWidth');
        if (!widthResponse.ok) {
            throw new Error('Network response was not ok');
        }
        let widthData = await widthResponse.json();

        let heightResponse = await fetch('/api/v1/GetDisplayHeight');
        if (!heightResponse.ok) {
            throw new Error('Network response was not ok');
        }
        let heightData = await heightResponse.json();

        displayWidth = widthData.displayWidth;
        displayHeight = heightData.displayHeight;

        // Update the DOM elements with the fetched dimensions
        document.querySelectorAll('.displayWidth').forEach(elem => elem.textContent = displayWidth);
        document.querySelectorAll('.displayHeight').forEach(elem => elem.textContent = displayHeight);

    } catch (error) {
        console.error('Error fetching display dimensions:', error);
        document.getElementById('displayError').textContent = 'Fehler beim Abrufen der Displayabmessungen: ' + error.message;

        // Display default width and height
        document.querySelectorAll('.displayWidth').forEach(elem => elem.textContent = ' - ');
        document.querySelectorAll('.displayHeight').forEach(elem => elem.textContent = ' - ');
    }
   }

   async function restart() {
    try {
    const response = await fetch('/api/v1/restart', {
    method: 'POST'
    });

    if (!response.ok) {
    throw new Error('Network response was not ok');
    }

    return 'Neustart erfolgreich!';
    } catch (error) {
    console.error('Error during restart:', error);
    return 'Neustart fehlgeschlagen.';
    }
   }

   function displayIndex() {
    //document.getElementById("htmlVorlage").style.display = "none";
    document.getElementById("htmlDesign").style.display = "none";
    document.getElementById("pngUpload").style.display = "none"; 
    document.getElementById("intro").style.display = "block";
   }

   function displayPngUpload() {
    //document.getElementById("htmlVorlage").style.display = "none";
    document.getElementById("intro").style.display = "none";
    document.getElementById("htmlDesign").style.display = "none"; 
    document.getElementById("pngUpload").style.display = "block"; 
   }

   function displayHtmlDesign() {
    document.getElementById("pngUpload").style.display = "none"
    document.getElementById("intro").style.display = "none";
    //document.getElementById("htmlVorlage").style.display = "none"; 
    document.getElementById("htmlDesign").style.display = "block"; 
   }

 /*  function displayHtmlVorlage() {
    document.getElementById("pngUpload").style.display = "none"
    document.getElementById("intro").style.display = "none";
    document.getElementById("htmlDesign").style.display = "none"; 
    document.getElementById("htmlVorlage").style.display = "block"; 
   }
*/

   function displayImage(event) {
    var file = event.target.files[0];
    var reader = new FileReader();
    reader.onload = function(e) {
        var img = document.createElement('img');
        img.src = e.target.result;
        img.style.maxWidth = '100%'; // Optional: Beschränke die Breite des Bildes
        var imageContainer = document.getElementById('imageContainer');
        imageContainer.innerHTML = ''; // Entferne vorherige Inhalte
        imageContainer.appendChild(img);
    }
    reader.readAsDataURL(file);
   }

   function pngUpload() {
    var fileInput = document.getElementById('fileInput');

    // Überprüfen, ob eine Datei ausgewählt wurde
    if (fileInput.files.length === 0) {
        alert('Bitte wählen Sie eine PNG-Datei aus.');
        return;
    }

    var file = fileInput.files[0];

    // Überprüfen, ob es sich um eine PNG-Datei handelt
    if (file.type !== 'image/png') {
        alert('Bitte wählen Sie eine PNG-Datei aus.');
        return;
    }

    // Erstellen eines Image-Objekts, um die Größe des PNG-Bildes zu überprüfen
    var img = new Image();
    img.onload = function() {
        // Überprüfen, ob die Breite und Höhe des Bildes 800x480px sind
        if (img.width !== displayWidth || img.height !== displayHeight) {
            alert('Das PNG ist nicht im richtigen Pixelformat für das angeschlossene Display');
            return;
        }

        // Wenn die Größe korrekt ist, weiterhin den Upload durchführen
        var formData = new FormData();
        formData.append('file', file, file.name);

        // Flag hinzufügen
        var ditheringCheckbox = document.getElementById('ditheringCheckbox');
        var flag = ditheringCheckbox.checked ? 'on' : 'off';
        formData.append('flag', flag);

        var url = '/api/v1/UploadImg';

        fetch(url, {
            method: 'POST',
            body: formData
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Fehler beim Senden der Daten. Statuscode: ' + response.status);
            }
            return response.json();
        })
        .then(data => {
            alert('Daten erfolgreich gesendet: ' + JSON.stringify(data));
        })
        .catch(error => {
            alert('Fehler beim Senden der Daten: ' + error.message);
        });
    };
    img.src = URL.createObjectURL(file);
   }

   function convertToHTML() {
        var inputText = document.getElementById("inputText").value;
        var preview = document.getElementById("preview");

        // Set the preview content
        preview.innerHTML = inputText;
   }

  /* function loadTemplate() {
    document.getElementById("inputText2").value = "Hallo!"
    console.log("Laden vom Template");
    var url = './layouts/Timetable_1.html';

    // Only working when properly hosted
    // // Configuration of the Fetch request
    // var requestOptions = {
    //     method: 'GET',
    // };
    //         fetch(url, requestOptions)
    //                     .then(function (response) {
    //                         // Check if the request was successful (status code 200)
    //                         if (!response.ok) {
    //                             throw new Error('Fehler beim Senden der Daten. Statuscode: ' + response.status);
    //                         }
    //                         document.getElementById("inputText").value = response.value();
    //                                 // return response.json(); // Return the JSON stream of the response
    //                         return response.value();
    //                     });
}
*/
   
   document.getElementById('inputText').addEventListener('input', () => {
        const inputText = document.getElementById("inputText").value;
        const preview = document.getElementById("preview");
        preview.innerHTML = inputText;
   });

   function convertAndUploadHtml() {
        var inputText = document.getElementById("inputText").value;
        var preview = document.getElementById("preview");

        
        html2canvas(preview, { scale: 1 }).then(canvas => {
            if (canvas.width !== displayWidth || canvas.height !== displayHeight) {
                alert('Ihr Design ist nicht im richtigen Pixelformat für das angeschlossene Display');
                return;
            }

                // Erstellen eines Blob-Objekts aus dem Canvas-Bild
                canvas.toBlob(function(blob) {
                    var url = '/api/v1/UploadImg';
                    var formData = new FormData();
                    formData.append('file', blob, 'html_conversion.png');

                 // Flag hinzufügen
                 var ditheringCheckbox = document.getElementById('ditheringCheckbox');
                 var flag = ditheringCheckbox.checked ? 'on' : 'off';
                 formData.append('flag', flag);

    
                    // Senden des Bildes an den Server
                    fetch(url, {
                        method: 'POST',
                        body: formData
                    })
                    .then(function(response) {
                        if (!response.ok) {
                            throw new Error('Fehler beim Senden der Daten. Statuscode: ' + response.status);
                        }
                        return response.json();
                    })
                    .then(function(data) {
                        // Erfolgsmeldung anzeigen
                        alert('Daten erfolgreich gesendet: ' + JSON.stringify(data));
                    })
                    .catch(function(error) {
                        // Fehlermeldung anzeigen
                        alert('Fehler beim Senden der Daten: ' + error.message);
                    });
                }, 'image/png');
            });
   }
