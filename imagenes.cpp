#include "imagenes.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace std;

namespace mri {
namespace {
void crearDirectorio(const filesystem::path& ruta) {
    if (ruta.has_parent_path()) filesystem::create_directories(ruta.parent_path());
}
}

void validarImagen(const cv::Mat& imagen) {
    if (imagen.empty() || imagen.dims != 2 || imagen.type() != CV_8UC1)
        throw invalid_argument("Se requiere una imagen 2D no vacia, gris de 8 bits");
}

cv::Mat cargarImagen(const filesystem::path& ruta) {
    cv::Mat imagen = cv::imread(ruta.string(), cv::IMREAD_UNCHANGED);
    if (imagen.empty()) throw runtime_error("No se pudo cargar: " + ruta.string());
    if (imagen.depth() != CV_8U)
        throw invalid_argument("La imagen debe tener 8 bits por canal: " + ruta.string());
    cv::Mat gris;
    if (imagen.channels() == 3) cv::cvtColor(imagen, gris, cv::COLOR_BGR2GRAY);
    else if (imagen.channels() == 4) cv::cvtColor(imagen, gris, cv::COLOR_BGRA2GRAY);
    else gris = imagen;
    validarImagen(gris);
    return gris;
}

cv::Mat redimensionar(const cv::Mat& imagen, int ancho, int alto) {
    validarImagen(imagen);
    if (ancho <= 0 || alto <= 0) throw invalid_argument("Dimensiones no positivas");
    cv::Mat salida;
    cv::resize(imagen, salida, cv::Size(ancho, alto), 0, 0, cv::INTER_AREA);
    return salida;
}

int intensidad(const cv::Mat& imagen, int fila, int columna) {
    validarImagen(imagen);
    if (fila < 0 || fila >= imagen.rows || columna < 0 || columna >= imagen.cols)
        throw out_of_range("Coordenadas fuera de la imagen");
    return imagen.ptr<unsigned char>(fila)[columna];
}

Histograma calcularHistograma(const cv::Mat& imagen) {
    validarImagen(imagen);
    Histograma histograma{};
    for (int f = 0; f < imagen.rows; ++f) {
        const auto* fila = imagen.ptr<unsigned char>(f);
        for (int c = 0; c < imagen.cols; ++c) ++histograma[fila[c]];
    }
    return histograma;
}

void guardarHistograma(const Histograma& histograma, const filesystem::path& ruta) {
    crearDirectorio(ruta);
    ofstream archivo(ruta);
    archivo << "intensidad,frecuencia\n";
    for (int g = 0; g < 256; ++g) archivo << g << ',' << histograma[g] << '\n';
    archivo.close();
    if (!archivo) throw runtime_error("No se pudo guardar: " + ruta.string());
}

void validarUmbrales(const vector<int>& umbrales) {
    if (umbrales.empty() || umbrales.size() > 254)
        throw invalid_argument("Se requieren entre 1 y 254 umbrales");
    int anterior = 0;
    for (int t : umbrales) {
        if (t <= anterior || t > 254)
            throw invalid_argument("Se requiere 1 <= t1 < ... < tk <= 254");
        anterior = t;
    }
}

Segmentacion segmentar(const cv::Mat& imagen, const vector<int>& umbrales) {
    validarImagen(imagen);
    validarUmbrales(umbrales);
    array<unsigned char, 256> etiquetas{}, tonos{};
    for (int g = 0; g < 256; ++g) {
        // Un pixel igual al umbral pertenece a la clase inferior.
        auto clase = lower_bound(umbrales.begin(), umbrales.end(), g) - umbrales.begin();
        etiquetas[g] = static_cast<unsigned char>(clase);
        tonos[g] = static_cast<unsigned char>(cvRound(255.0 * clase / umbrales.size()));
    }
    Segmentacion salida{cv::Mat(imagen.size(), CV_8UC1), cv::Mat(imagen.size(), CV_8UC1)};
    for (int f = 0; f < imagen.rows; ++f) {
        const auto* entrada = imagen.ptr<unsigned char>(f);
        auto* clases = salida.etiquetas.ptr<unsigned char>(f);
        auto* visual = salida.visualizacion.ptr<unsigned char>(f);
        for (int c = 0; c < imagen.cols; ++c) {
            clases[c] = etiquetas[entrada[c]];
            visual[c] = tonos[entrada[c]];
        }
    }
    return salida;
}

void guardarImagen(const cv::Mat& imagen, const filesystem::path& ruta) {
    validarImagen(imagen);
    auto extension = ruta.extension().string();
    transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return tolower(c); });
    if (extension != ".png") throw invalid_argument("Guardar con extension .png");
    crearDirectorio(ruta);
    if (!cv::imwrite(ruta.string(), imagen))
        throw runtime_error("No se pudo guardar: " + ruta.string());
}
} // namespace mri
