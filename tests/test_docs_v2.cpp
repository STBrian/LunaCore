/* @doc:v2
@module TestModule
@desc Modulo de prueba para el nuevo formato v2 de documentacion
*/

/* @doc:v2
@class Vector2D
@desc Representa un vector bidimensional
@field x number Coordenada X
@field y number Coordenada Y
*/

/* @doc:v2
@class PlayerEntity: TestModule
@desc Entidad de jugador derivada del modulo
@field id integer Identificador unico
@field name string Nombre del jugador
*/

/* @doc:v2
@func TestModule.calculateDistance(a: Vector2D, b: Vector2D): number
@desc Calcula la distancia euclidiana entre dos vectores
@param a Vector2D Primer punto
@param b Vector2D Segundo punto
@return number Distancia calculada
*/

/* @doc:v2
@method PlayerEntity:teleport(x: number, y: number, [smooth]: boolean): boolean
@desc Teletransporta al jugador a las coordenadas indicadas
@param x number Posicion de destino X
@param y number Posicion de destino Y
@param smooth boolean? Si la transicion debe ser suave
@return boolean True si la operacion fue exitosa
@deprecated Use setPosition instead
@see PlayerEntity:setPosition
*/

/* @doc:v2
@func TestModule.log(msg: string, ...: any): void
@desc Registra un mensaje con argumentos variables
@param msg string Mensaje base
@param ... any Parametros variables de formato
*/

/* @doc:v2
@const TestModule.MAX_PLAYERS = 16
@desc Numero maximo de jugadores simultaneos
*/

/* @doc:v2
@raw ---@alias CallbackFunction fun(event: string): boolean
*/
