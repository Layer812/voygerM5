#pragma once
// A small group of real J2000 bright-star directions, supplemented by synthetic faint sky elsewhere.
struct BrightStar {float ra,dec,mag;};
static constexpr BrightStar BRIGHT_STARS[]={
 {101.287f,-16.716f,-1.46f}, // Sirius
 {95.987f,-52.696f,-.74f}, // Canopus
 {213.915f,19.182f,-.05f}, // Arcturus
 {279.235f,38.784f,.03f}, // Vega
 {79.172f,45.998f,.08f}, // Capella
 {78.634f,-8.202f,.13f}, // Rigel
 {114.826f,5.225f,.34f}, // Procyon
 {88.793f,7.407f,.42f}, // Betelgeuse
 {24.429f,-57.237f,.46f}, // Achernar
 {210.956f,-60.373f,.61f}, // Hadar
 {186.650f,-63.099f,.76f}, // Acrux
 {201.298f,-11.161f,.98f}, // Spica
 {247.352f,-26.432f,1.06f}, // Antares
 {68.980f,16.509f,.85f}, // Aldebaran
 {116.329f,28.026f,1.14f}, // Pollux
 {344.413f,-29.622f,1.16f}, // Fomalhaut
 {310.358f,45.280f,1.25f}, // Deneb
 {297.696f,8.868f,.77f}, // Altair
 {152.093f,11.967f,1.35f}, // Regulus
 {37.954f,89.264f,1.98f}, // Polaris
 {81.283f,6.350f,1.64f}, // Bellatrix
 {84.053f,-1.202f,1.69f}, // Alnilam
 {332.058f,-46.961f,1.74f}, // Alnair
 {191.930f,-59.689f,1.25f}, // Mimosa
 {165.932f,61.751f,1.79f}, // Dubhe
 {165.460f,56.383f,2.37f}, // Merak
 {51.081f,49.861f,1.79f}, // Mirfak
 {206.885f,49.313f,1.85f}, // Alkaid
 {81.572f,28.608f,1.65f}, // Elnath
 {200.981f,54.925f,2.23f}, // Mizar
 {305.557f,40.256f,2.23f}, // Sadr
 {263.402f,-37.104f,1.62f}, // Shaula
 {283.816f,-34.384f,1.79f}, // Kaus Australis
 {113.65f,31.888f,1.58f}, // Castor
 {177.264f,14.572f,2.14f}, // Denebola
};
