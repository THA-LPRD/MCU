
var byteArray = [];


window.onload = function() {
    displayIndex();
    getDisplayDimensions()
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


document.addEventListener('DOMContentLoaded', function() {

    // Funktion zum Anzeigen der Bilder mit Uhrzeiten
    function displayImagesWithTimes(data) {
        var imageContainer = document.getElementById('imageContainer');
        imageContainer.innerHTML = '';

        for (var i = 0; i < data.length; i++) {
            var item = data[i];
            var fileName = item.name;
            var time = item.value;

            var imgContainer = document.createElement('div');
            imgContainer.classList.add('col'); // CSS-Klasse für eine Spalte

            var img = document.createElement('img');
            img.src = '/api/v1/uploadpng/' + fileName; // Pfade für die Bilder aus der API anpassen
            img.alt = 'Uploaded Image';
            img.style.maxWidth = '200px';
            img.style.margin = '10px';

            var fileNameElem = document.createElement('p');
            fileNameElem.textContent = fileName; // Anzeigen des Dateinamens unter dem Bild

            var timeElem = document.createElement('p');
            timeElem.textContent = 'Uhrzeit: ' + time; // Anzeigen der Uhrzeit unter dem Bild

            imgContainer.appendChild(img);
            imgContainer.appendChild(fileNameElem); // Hinzufügen des Dateinamens
            imgContainer.appendChild(timeElem); // Hinzufügen der Uhrzeit

            imageContainer.appendChild(imgContainer);
        }
    }

    // Funktion zum Abrufen der Daten und Anzeigen der Bilder mit Uhrzeiten
    function fetchImagesWithTimes() {
        fetch('/api/v1/inputfile')
            .then(function(response) {
                if (!response.ok) {
                    throw new Error('Fehler beim Abrufen der Daten. Statuscode: ' + response.status);
                }
                return response.json();
            })
            .then(function(data) {
                console.log('Daten erfolgreich abgerufen:', data);
                displayImagesWithTimes(data);
            })
            .catch(function(error) {
                console.error('Fehler beim Abrufen der Daten:', error);
            });
    }

    // Funktion zum Initialisieren
    function initialize() {
        fetchImagesWithTimes(); // Bilder mit Uhrzeiten anzeigen
    }

    initialize();



    // Array zum Speichern ausgewählter Dateien
    var selectedFiles = [];

    document.getElementById('fileInput').addEventListener('change', function() {
        addSelectedFiles(this.files);
        displaySelectedFiles();
        this.value = null; // Reset the file input value so that the same file can be uploaded again
    });

    document.getElementById('addButton').addEventListener('click', function() {
        document.getElementById('fileInput').click(); // Öffnet den Dateiauswahldialog
    });

    document.getElementById('uploadButton').addEventListener('click', function() {
        convertAndUploadPng();
    });

    function initializeFileInput() {
        var fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.id = 'fileInput';
        fileInput.accept = 'image/png';
        fileInput.multiple = true;

        fileInput.addEventListener('change', function() {
            addSelectedFiles(this.files);
            displaySelectedFiles();
        });

        document.body.appendChild(fileInput);
    }


    function generateUniqueId() {
        return Date.now().toString() + Math.random().toString(36).substring(7);
    }

    
    // Funktion zum Hochladen der ausgewählten Dateien
    function uploadSelectedFiles() {
        convertAndUploadPng(selectedFiles);
    }

    

    function addSelectedFiles(files) {
        for (var i = 0; i < files.length; i++) {
            var file = files[i];
            if (file.type === 'image/png') {
                var uniqueId = generateUniqueId();
                selectedFiles.push({ file: file, id: uniqueId, value: '', isValid: true }); // Eindeutige Kennung für jedes Bild hinzufügen
            } else {
                alert('Bitte wählen Sie nur PNG-Dateien aus.');
            }
        }
    }


    function removeSelectedFile(index) {
        selectedFiles.splice(index, 1);
        displaySelectedFiles();
    }


    function displaySelectedFiles() {
        var imageContainer = document.getElementById('imageContainer');
        imageContainer.innerHTML = '';

        var row;
        var counter = 0;

        for (var i = 0; i < selectedFiles.length; i++) {
            var fileObj = selectedFiles[i];
            var file = fileObj.file;
            var value = fileObj.value;

            if (counter % 3 === 0) {
                row = document.createElement('div');
                row.classList.add('row'); // CSS-Klasse für eine Reihe mit Bildern
                imageContainer.appendChild(row);
            }

            var imgContainer = document.createElement('div');
            imgContainer.classList.add('col'); // CSS-Klasse für eine Spalte

            var img = document.createElement('img');
            img.src = URL.createObjectURL(file);
            img.alt = 'Uploaded Image';
            img.style.maxWidth = '200px';
            img.style.margin = '10px';

            var fileName = document.createElement('p');
            fileName.textContent = file.name; // Anzeigen des Dateinamens unter dem Bild

            var input = document.createElement('input');
            input.type = 'text';
            input.placeholder = '00:00 Uhr';
            input.value = value;
            input.classList.add('inputUhrzeit'); // Hinzufügen der Klasse .input-text
            input.addEventListener('blur', (function(index, container) {
                return function(e) {
                    var inputValue = e.target.value;
                    var isValid = validateTime(inputValue);

                    selectedFiles[index].isValid = isValid;
                    selectedFiles[index].value = inputValue; // Aktualisiere den Wert im Array, wenn das Textfeld geändert wird

                    renderValidationMessage(container, isValid);
                };
            })(i, imgContainer));

            // Überprüfe initiale Validität und rendere die Fehlermeldung
            renderValidationMessage(imgContainer, fileObj.isValid);

            var removeButton = document.createElement('button');
            removeButton.textContent = 'Bild entfernen';
            removeButton.classList.add('entfernenButton'); // Hinzufügen der Klasse .inputUhrzeit
            removeButton.addEventListener('click', (function(index) {
                return function() {
                    removeSelectedFile(index);
                    displaySelectedFiles(); // Nach dem Entfernen neu rendern
                };
            })(i));

            imgContainer.appendChild(img);
            imgContainer.appendChild(fileName); // Hinzufügen des Dateinamens
            imgContainer.appendChild(input); // Hinzufügen des Textfelds
            imgContainer.appendChild(removeButton);

            row.appendChild(imgContainer);

            counter++;
        }
    }

    function renderValidationMessage(container, isValid) {
        var invalidMsg = container.querySelector('.invalidMessage');
        if (!isValid) {
            if (!invalidMsg) {
                invalidMsg = document.createElement('span');
                invalidMsg.textContent = 'Ungültige Eingabe, bitte die Uhrzeit im Format HH:MM eingeben';
                invalidMsg.classList.add('invalidMessage');
                invalidMsg.style.color = 'red'; // Farbe ändern
                invalidMsg.style.display = 'block'; // Zeilenumbruch hinzufügen
                invalidMsg.style.maxWidth = '200px'; // Maximale Breite setzen
                container.appendChild(invalidMsg); // Fehlermeldung am Ende des Containers hinzufügen
            }
        } else {
            if (invalidMsg) {
                invalidMsg.remove();
            }
        }
    }

    function validateTime(time) {
        // Überprüfung der Uhrzeit auf Gültigkeit
        if (time === '') {
            return true; // Leere Eingabe ist gültig
        }
        var timePattern = /^([01]\d|2[0-3]):([0-5]\d)$/;
        return timePattern.test(time);
    }

    function convertAndUploadPng() {
        // Überprüfung, ob alle Eingaben gültig sind
        var allValid = selectedFiles.every(function(fileObj) {
            return fileObj.isValid;
        });
    
        if (!allValid) {
            alert('Upload fehlgeschlagen: Bitte überprüfen Sie die Uhrzeiteingaben.');
            return;
        }
    
        // JSON mit Dateinamen und Werten erstellen
        var jsonData = selectedFiles.map(function(fileObj) {
            return {
                name: fileObj.file.name,
                value: fileObj.value
            };
        });
    
        console.log('JSON-Daten:', jsonData);

        var url = '/api/v1/inputfile';
    
        var requestOptions = {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ data: jsonData }) // Name der Datei hinzufügen
        };

        fetch(url, requestOptions)
            .then(function(response) {
                if (!response.ok) {
                    throw new Error('Fehler beim Senden der Daten. Statuscode: ' + response.status);
                }
                return response.json();
            })
            .then(function(data) {
                console.log('Daten erfolgreich gesendet:', data);
            })
            .catch(function(error) {
                console.error('Fehler beim Senden der Daten:', error);
            });
    
        // Iteriere über die ausgewählten Dateien, um sie zu konvertieren und hochzuladen
        for (var i = 0; i < selectedFiles.length; i++) {
            var fileObj = selectedFiles[i];
            var file = fileObj.file;
            var reader = new FileReader();

        // PNG-Bild verkleinern und hochladen
        resizeAndUploadPng(fileObj);
    
            reader.onload = (function(fileObj) {
                return function(e) {
                    var img = new Image();
                    img.onload = function() {
                        var canvas = document.createElement('canvas');
                        canvas.width = this.width;
                        canvas.height = this.height;
    
                        var ctx = canvas.getContext('2d');
                        ctx.drawImage(this, 0, 0);
    
                        var imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
                        var byteArray = convertImageDataToByteArray(imageData);
    
                        console.log('Gesendete Daten:', JSON.stringify({ data: byteArray, name: fileObj.file.name })); // Ausgabe der zu sendenden Daten
    
                        var url = '/api/v1/uploadbmp';
    
                        var requestOptions = {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json'
                            },
                            body: JSON.stringify({ data: byteArray, name: fileObj.file.name, value: fileObj.value }) // Name der Datei hinzufügen
                        };
    
                        fetch(url, requestOptions)
                            .then(function(response) {
                                if (!response.ok) {
                                    throw new Error('Fehler beim Senden der Daten. Statuscode: ' + response.status);
                                }
                                return response.json();
                            })
                            .then(function(data) {
                                console.log('Daten erfolgreich gesendet:', data);
                            })
                            .catch(function(error) {
                                console.error('Fehler beim Senden der Daten:', error);
                            });
                    };
                    img.src = e.target.result;
                };
            })(fileObj);

            
            reader.readAsDataURL(file);
        }


    }

});





    function resizeAndUploadPng(fileObj) {
    var file = fileObj.file;
    var reader = new FileReader();

    reader.onload = function(e) {
        var img = new Image();
        img.onload = function() {
            // Größe des Bildes um 25% reduzieren
            var newWidth = img.width * 0.25;
            var newHeight = img.height * 0.25;

            var canvas = document.createElement('canvas');
            canvas.width = newWidth;
            canvas.height = newHeight;

            var ctx = canvas.getContext('2d');
            ctx.drawImage(this, 0, 0, newWidth, newHeight);

            canvas.toBlob(function(blob) {
                console.log('Gesendete Daten (PNG verkleinert um 75%):', blob);

                var url = '/api/v1/uploadpng';

                var formData = new FormData();
                formData.append('file', blob, file.name);

                var requestOptions = {
                    method: 'POST',
                    body: formData
                };

                fetch(url, requestOptions)
                    .then(function(response) {
                        if (!response.ok) {
                            throw new Error('Fehler beim Senden der Daten. Statuscode: ' + response.status);
                        }
                        return response.json();
                    })
                    .then(function(data) {
                        console.log('Daten erfolgreich gesendet:', data);
                    })
                    .catch(function(error) {
                        console.error('Fehler beim Senden der Daten:', error);
                    });
            }, 'image/png');
        };
        img.src = e.target.result;
    };

    reader.readAsDataURL(file);
    }


    function convertToHTML() {
        var inputText = document.getElementById("inputText").value;
        var preview = document.getElementById("preview");

        // Set the preview content
        preview.innerHTML = inputText;
    }


    function convertAndUpload() {
        html2canvas(document.querySelector("#preview"), { scale: 1 }).then(canvas => {
            var img = canvas.toDataURL("image/png");
            document.getElementById('previewImg').src = img;
            var context = canvas.getContext("2d");
            console.log(createImageBitmap(document.getElementById('previewImg'), 0, 0, 800, 480).data);
            var context = canvas.getContext("2d");
            var imageData = context.getImageData(0, 0, canvas.width, canvas.height);

            console.log(canvas.width, canvas.height);

            byteArray = convertImageDataToByteArray(imageData);
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
    