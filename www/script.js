window.onload = function() {
    displayIndex();
    getDisplayDimensions()
}

async function getDisplayDimensions() {
    try {
        let widthResponse = await fetch('http://127.0.0.1:5000/api/v1/getDisplayWidth');
        if (!widthResponse.ok) {
            throw new Error('Network response was not ok');
        }
        let widthData = await widthResponse.json();

        let heightResponse = await fetch('http://127.0.0.1:5000/api/v1/getDisplayHeight');
        if (!heightResponse.ok) {
            throw new Error('Network response was not ok');
        }
        let heightData = await heightResponse.json();

        let displayWidth = widthData.displayWidth;
        let displayHeight = heightData.displayHeight;

        // Set the dimensions of the previewContainer
        let previewContainer = document.getElementById('previewContainer');

        if (displayWidth > 800) {
            // Calculate scaled height to fit within 800px width
            let scaledHeight = (850 / displayWidth) * displayHeight;
            previewContainer.style.width = '800px';
            previewContainer.style.height = scaledHeight + 'px';
        } else {
            // Set dimensions as received from the server
            previewContainer.style.width = displayWidth + 'px';
            previewContainer.style.height = displayHeight + 'px';
        }

        // Display width and height
        document.getElementById('displayWidth').textContent = previewContainer.style.width;
        document.getElementById('displayHeight').textContent = previewContainer.style.height;

    } catch (error) {
        console.error('Error fetching display dimensions:', error);
        // Set default dimensions if there's an error
        let previewContainer = document.getElementById('previewContainer');
        previewContainer.style.width = '800px';
        previewContainer.style.height = '480px';
        // Display default width and height
        document.getElementById('displayWidth').textContent = '800';
        document.getElementById('displayHeight').textContent = '480';
    }
}


   function displayIndex() {
    document.getElementById("htmlUpload").style.display = "none";
    document.getElementById("pngUpload").style.display = "none"; 
    document.getElementById("intro").style.display = "block";
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
    if (fileInput.files.length > 0) {
        var file = fileInput.files[0];

        // Überprüfen, ob es sich um eine PNG-Datei handelt
        if (file.type !== 'image/png') {
            // Fehlermeldung anzeigen und Upload abbrechen
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

            var url = 'http://127.0.0.1:5000/api/v1/uploadpng';

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
    } else {
        alert('Bitte wählen Sie eine PNG-Datei aus.');
    }
}


    function convertToHTML() {
        var inputText = document.getElementById("inputText").value;
        var preview = document.getElementById("preview");

        // Set the preview content
        preview.innerHTML = inputText;
    }

    function scaleHTML() {
        var inputText = document.getElementById("inputText").value;
        var preview = document.getElementById("preview");
    
        // Erstelle ein unsichtbares Element, um die Breite des HTML-Codes zu messen
        var tempDiv = document.createElement('div');
        tempDiv.innerHTML = inputText;
        tempDiv.style.visibility = 'hidden';
        tempDiv.style.position = 'absolute';
        tempDiv.style.width = 'auto';
        document.body.appendChild(tempDiv);
    
        // Messen der Breite des HTML-Codes
        var width = tempDiv.offsetWidth;
    
        // Skalierung der Breite auf maximal 850px
        var scaleFactor = 1;
        if (width > 800) {
            scaleFactor = 800 / width;
        }
    
        // Anwenden der Skalierung
        preview.innerHTML = inputText;
        preview.style.transform = 'scale(' + scaleFactor + ')';
        preview.style.transformOrigin = 'top left';
    
        // Entfernen des temporären Elements
        document.body.removeChild(tempDiv);
    }

    function convertAndUploadHtml() {
        var inputText = document.getElementById("inputText").value;
        var preview = document.getElementById("preview");
    
        // Erstelle ein unsichtbares Element, um die Breite des HTML-Codes zu messen
        var tempDiv = document.createElement('div');
        tempDiv.innerHTML = inputText;
        tempDiv.style.visibility = 'hidden';
        tempDiv.style.position = 'absolute';
        tempDiv.style.width = 'auto';
        document.body.appendChild(tempDiv);
    
        // Messen der Breite des HTML-Codes
        var width = tempDiv.offsetWidth;
    
        // Entfernen des temporären Elements
        document.body.removeChild(tempDiv);
    
        // Überprüfen, ob die Breite größer als 800px ist
        if (width > 800) {
            // Wenn die Breite größer als 800px ist, skalieren und senden
            html2canvas(previewContainer, { scale: width / 800 }).then(canvas => {
                // Erstellen eines Blob-Objekts aus dem Canvas-Bild
                canvas.toBlob(function(blob) {
                // Erstellen eines neuen Canvas-Elements für das zugeschnittene Bild
                var croppedCanvas = document.createElement('canvas');
                var croppedContext = croppedCanvas.getContext('2d');

                // Setze die Größe des zugeschnittenen Canvas auf die gewünschte Größe
                croppedCanvas.width = displayWidth; // von Server vorgegebene Breite
                croppedCanvas.height = displayHeight; // von Server vorgegebene Höhe

                // Zuschneiden des Bildes auf die gewünschte Größe
                croppedContext.drawImage(canvas, 0, 0, croppedCanvas.width, croppedCanvas.height);

                // Erstellen eines Blob-Objekts aus dem zugeschnittenen Canvas-Bild
                croppedCanvas.toBlob(function(croppedBlob) {
                    var url = 'http://127.0.0.1:5000/api/v1/uploadpng';
                    var formData = new FormData();
                    formData.append('file', croppedBlob, 'html_conversion.png');
    
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
            }, 'image/png');
        });
        } else {
            // Wenn die Breite nicht größer als 800px ist, einfach das HTML-Bild hochladen
            html2canvas(preview, { scale: 1 }).then(canvas => {
                // Erstellen eines Blob-Objekts aus dem Canvas-Bild
                canvas.toBlob(function(blob) {
                    var url = 'http://127.0.0.1:5000/api/v1/uploadpng';
                    var formData = new FormData();
                    formData.append('file', blob, 'html_conversion.png');
    
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
    }
    
    
    
 