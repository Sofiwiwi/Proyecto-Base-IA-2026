#pragma once

#include <opencv2/core.hpp>
#include <array>
#include <filesystem>
#include <vector>

namespace mri {
using Histograma = std::array<unsigned long long, 256>;

// Todas las operaciones requieren una matriz no vacia CV_8UC1.
void validarImagen(const cv::Mat& imagen);
// Lee sin reducir profundidad; convierte BGR/BGRA de 8 bits a gris.
// Rechaza imagenes de 16 bits para evitar conversiones implicitas de intensidad.
cv::Mat cargarImagen(const std::filesystem::path& ruta);
cv::Mat redimensionar(const cv::Mat& imagen, int ancho = 256, int alto = 256);
int intensidad(const cv::Mat& imagen, int fila, int columna);
Histograma calcularHistograma(const cv::Mat& imagen);
void guardarHistograma(const Histograma& histograma, const std::filesystem::path& ruta);
void validarUmbrales(const std::vector<int>& umbrales);

struct Segmentacion {
    cv::Mat etiquetas;     // Valores 0,...,k: identificadores de clase.
    cv::Mat visualizacion; // Valores equiespaciados entre 0 y 255.
};
// Clases inclusivas: [0,t1], [t1+1,t2], ..., [tk+1,255].
// Acepta cualquier cantidad de umbrales entre 1 y 254, incluidos k=2,4,6.
Segmentacion segmentar(const cv::Mat& imagen, const std::vector<int>& umbrales);
// Exige extension .png (sin perdida) y crea los directorios necesarios.
void guardarImagen(const cv::Mat& imagen, const std::filesystem::path& ruta);
} // namespace mri
