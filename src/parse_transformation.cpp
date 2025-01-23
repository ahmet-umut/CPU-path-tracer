#include "parse_transformation.hh"
#include "vector_utilities.hh"
matrix parse_transformation(xmlNode *node, Tempor&tempor)
{
	matrix transformation = matrix::identity();

    if (node == nullptr) return transformation;

    // Assuming node content is something like "r1 t2 s3"
    string text = reinterpret_cast<const char*>(xmlNodeGetContent(node));
    std::istringstream iss(text);
    string token;

    while (iss >> token) {
        if (!token.empty()) {
            char type = token[0];  // 'r', 't', or 's'
            int id = std::stoi(token.substr(1));  // 1, 2, 3, ...

            int indice;
            switch (type) {
                case 'r':
                    indice = 0;
                    break;
                case 't':
                    indice = 2;
                    break;
                case 's':
                    indice = 1;
                    break;
                case 'c':
                    indice = 3;
                    break;
                default:
                    cerr << "Invalid transformation type: " << type << endl;
                    return transformation;
            }
			transformation = tempor.transformations[indice][id - 1] * transformation;
        }
    }
    return transformation;
}