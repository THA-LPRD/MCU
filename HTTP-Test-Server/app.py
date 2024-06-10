from flask import Flask, request, jsonify, send_file
import os
import uuid
import io

app = Flask(__name__)

UPLOAD_FOLDER = 'uploads'
if not os.path.exists(UPLOAD_FOLDER):
    os.makedirs(UPLOAD_FOLDER)

@app.route('/')
def index():
    return "Server is running"

@app.route('/api/v1/uploadpng', methods=['POST'])
def upload_png():
    name = request.args.get('name', default=str(uuid.uuid4()) + '.png')
    file = request.files['file']

    png_path = os.path.join(UPLOAD_FOLDER, name)
    file.save(png_path)

    return jsonify({"message": "PNG file uploaded successfully", "path": png_path})

@app.route('/uploads/<filename>')
def uploaded_file(filename):
    return send_file(os.path.join(UPLOAD_FOLDER, filename), mimetype='image/png')

if __name__ == '__main__':
    app.run(debug=True, port=5000)

