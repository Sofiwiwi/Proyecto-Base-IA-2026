#include "imagenes.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

int main(int argc, char** argv) {
    if (argc < 4) {
        cerr << "Uso: " << argv[0] << " imagen carpeta_salida t1 [t2 ... tk]\n";
        return 1;
    }
    try {
        vector<int> umbrales;
        for (int i = 3; i < argc; ++i) {
            string argumento = argv[i];
            size_t usados = 0;
            int t = stoi(argumento, &usados);
            if (usados != argumento.size()) throw invalid_argument("Umbral no entero");
            umbrales.push_back(t);
        }
        mri::validarUmbrales(umbrales);
        const auto imagen = mri::cargarImagen(argv[1]);
        const auto histograma = mri::calcularHistograma(imagen);
        const auto resultado = mri::segmentar(imagen, umbrales);
        const filesystem::path salida = argv[2];
        mri::guardarImagen(resultado.etiquetas, salida / "etiquetas.png");
        mri::guardarImagen(resultado.visualizacion, salida / "segmentada.png");
        mri::guardarHistograma(histograma, salida / "histograma.csv");
        cout << "Imagen: " << imagen.cols << 'x' << imagen.rows
                  << ", intensidad (0,0): " << mri::intensidad(imagen, 0, 0)
                  << ", clases: " << umbrales.size() + 1 << "\nSalida: " << salida << '\n';
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
