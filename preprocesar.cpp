#include <opencv2/opencv.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
namespace fs = filesystem;

const int CANTIDAD_IMAGENES = 10;
const int ANCHO = 256;
const int ALTO = 256;
const int NIVELES_GRIS = 256;
const vector<int> VALORES_K = {2, 4, 6};

bool extensionValida(const fs::path& ruta) {
    string ext = ruta.extension().string();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".jpg" || ext == ".jpeg";
}

vector<fs::path> buscarImagenes(const fs::path& carpetaDataset) {
    vector<fs::path> imagenes;
    if (!fs::exists(carpetaDataset)) {
        throw runtime_error("La carpeta del dataset no existe: " + carpetaDataset.string());
    }
    for (const auto& entrada : fs::recursive_directory_iterator(carpetaDataset)) {
        if (entrada.is_regular_file() && extensionValida(entrada.path())) {
            imagenes.push_back(entrada.path());
        }
    }
    sort(imagenes.begin(), imagenes.end());
    return imagenes;
}

vector<fs::path> seleccionarImagenes(const vector<fs::path>& imagenes, unsigned int semilla) {
    if (static_cast<int>(imagenes.size()) < CANTIDAD_IMAGENES) {
        throw runtime_error(
            "Se necesitan al menos " + to_string(CANTIDAD_IMAGENES) +
            " imagenes JPG, pero se encontraron " + to_string(imagenes.size()) + "."
        );
    }
    vector<fs::path> copia = imagenes;
    mt19937 generador(semilla);
    shuffle(copia.begin(), copia.end(), generador);
    copia.resize(CANTIDAD_IMAGENES);
    sort(copia.begin(), copia.end());
    return copia;
}

cv::Mat preprocesarImagen(const fs::path& rutaEntrada) {
    cv::Mat imagen = cv::imread(rutaEntrada.string(), cv::IMREAD_GRAYSCALE);
    if (imagen.empty()) {
        throw runtime_error("No se pudo leer la imagen: " + rutaEntrada.string());
    }
    cv::Mat redimensionada;
    cv::resize(imagen, redimensionada, cv::Size(ANCHO, ALTO), 0.0, 0.0, cv::INTER_AREA);
    if (redimensionada.type() != CV_8UC1) {
        cv::Mat convertida;
        redimensionada.convertTo(convertida, CV_8UC1);
        return convertida;
    }
    return redimensionada;
}

array<unsigned long long, NIVELES_GRIS> calcularHistograma(const cv::Mat& imagen) {
    array<unsigned long long, NIVELES_GRIS> histograma{};
    histograma.fill(0);
    for (int fila = 0; fila < imagen.rows; ++fila) {
        const unsigned char* ptr = imagen.ptr<unsigned char>(fila);
        for (int col = 0; col < imagen.cols; ++col) {
            ++histograma[ptr[col]];
        }
    }

    return histograma;
}

void guardarInstancia(const fs::path& rutaSalida, const string& nombreImagen, int k, const array<unsigned long long, NIVELES_GRIS>& histograma) {
    ofstream archivo(rutaSalida);
    if (!archivo.is_open()) {
        throw runtime_error("No se pudo crear la instancia: " + rutaSalida.string());
    }
    // Formato deliberadamente simple: solo datos de entrada.
    // No contiene soluciones, umbrales, evaluaciones ni informacion
    // sobre como implementar la metaheuristica.
    archivo << "SEGMENTACION_MULTIUMBRAL_V1\n";
    archivo << "imagen " << nombreImagen << "\n";
    archivo << "ancho " << ANCHO << "\n";
    archivo << "alto " << ALTO << "\n";
    archivo << "L " << NIVELES_GRIS << "\n";
    archivo << "k " << k << "\n";
    archivo << "N " << (ANCHO * ALTO) << "\n";
    archivo << "histograma\n";

    for (int g = 0; g < NIVELES_GRIS; ++g) {
        archivo << histograma[g];
        if (g + 1 < NIVELES_GRIS) {
            archivo << ' ';
        }
    }
    archivo << '\n';
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        cerr << "Uso:\n";
        cerr << "  " << argv[0] << " <carpeta_dataset> <carpeta_salida> [semilla]\n\n";
        cerr << "Ejemplo:\n";
        cerr << "  " << argv[0] << " ./brain_tumor_dataset ./salida 42\n";
        return 1;
    }

    try {
        fs::path carpetaDataset = argv[1];
        fs::path carpetaSalida = argv[2];
        unsigned int semilla = (argc == 4) ? static_cast<unsigned int>(stoul(argv[3])) : 42u;

        fs::path carpetaImagenes = carpetaSalida / "imagenes";
        fs::path carpetaInstancias = carpetaSalida / "instancias";

        fs::create_directories(carpetaImagenes);
        fs::create_directories(carpetaInstancias);

        vector<fs::path> disponibles = buscarImagenes(carpetaDataset);
        vector<fs::path> seleccionadas = seleccionarImagenes(disponibles, semilla);

        cout << "Imagenes JPG encontradas: " << disponibles.size() << '\n';
        cout << "Imagenes seleccionadas: " << seleccionadas.size() << "\n\n";

        for (int i = 0; i < CANTIDAD_IMAGENES; ++i) {
            const fs::path& rutaOriginal = seleccionadas[i];

            ostringstream nombreBase;
            nombreBase << "imagen_" << setw(2) << setfill('0') << (i + 1);

            string nombreImagen = nombreBase.str() + ".jpg";
            fs::path rutaProcesada = carpetaImagenes / nombreImagen;

            cv::Mat imagen = preprocesarImagen(rutaOriginal);

            vector<int> parametrosJpg = {
                cv::IMWRITE_JPEG_QUALITY, 100
            };

            if (!cv::imwrite(rutaProcesada.string(), imagen, parametrosJpg)) {
                throw runtime_error("No se pudo guardar: " + rutaProcesada.string());
            }

            // Se vuelve a leer el JPG guardado para que el histograma de las
            // instancias corresponda exactamente al archivo entregado.
            cv::Mat imagenFinal = cv::imread(rutaProcesada.string(), cv::IMREAD_GRAYSCALE);
            if (imagenFinal.empty()) {
                throw runtime_error("No se pudo releer: " + rutaProcesada.string());
            }

            array<unsigned long long, NIVELES_GRIS> histograma = calcularHistograma(imagenFinal);

            for (int k : VALORES_K) {
                string nombreInstancia = nombreBase.str() + "_k" + to_string(k) + ".txt";
                guardarInstancia(
                    carpetaInstancias / nombreInstancia,
                    nombreImagen,
                    k,
                    histograma
                );
            }

            cout << "[" << setw(2) << setfill('0') << (i + 1) << "/10] "
                 << rutaOriginal.filename().string() << " -> " << nombreImagen << '\n';
        }

        cout << "\nPreprocesamiento finalizado.\n";
        cout << "Imagenes: 10\n";
        cout << "Resolucion: 256 x 256\n";
        cout << "Profundidad: 8 bits\n";
        cout << "k: 2, 4, 6\n";
        cout << "Instancias generadas: 30\n";
        cout << "Salida: " << fs::absolute(carpetaSalida).string() << '\n';

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
