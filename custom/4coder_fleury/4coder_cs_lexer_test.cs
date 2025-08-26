// 750 decl: a + b;
// 467 decl: if (a) b = ...;
// 459 not decl: var a = ...;
// 460 decl: public Constructor(int decl, int other);
// 457 not decl: List<T> a;
// 469 decl: a < b && b > c;
// 468 not decl: A<B<C>> a;
// 469 not decl: int a, b, c, d;
// 486 2 decl: enum { A = asdsfasd } and void Func() => { ... } isn't parsed properly
// 506 hasn't parsed function arguments
// 577 hasn't parsed single arguments and last arguments
// 633 no scope note
// 812 2 decl: new Vector2(targetDir.x * speed, rb.velocity.y)
// 798 no decl: for (int i = 0; i < ...; ++i)
// 806 not decl: SomeFunc(out Vector3Int groundPos) and foreach loop decl;
// 827 for simplicity I have remove parsing pointer *
// 840 single line type: struct TestStruct; and 1 decl: (..., out A a, b);
// 842 comments inside enum
// 843 skip lambda
// 880 parse lambda
// 883 parse lambda inside one-statement lambda
// 909 parse lambda argument
// 942 parse constructor and lambda in named argument: A(arg: () => ...);
// 971 parse operator overloading
// 973 parse macro define
// 986 parse property/getter/setter
// 1032 parse namespace and generic types, functions, and arguments
// 1042 fix comment after note.scope_range and comment's duplication
// 1045 parse "using"
// 1059 parse inheritance and constructors
// 1079 differentiate function calls with types
// 1078 fix generic parsing
// 1083 fix namespace handling
// 1091 parse generic arguments in constructors and operators
using UnityEngine;

namespace UnityEngine { class MyEngine; }
MyEngine myEngine;

#error My Error
#warning My Warning
#region My Region

namespace MyNamespace int a = 5;

namespace MyNamespace
{
    class A
    {
        namespace NamespaceInClass // NOTE(long): Classes can't have namespaces
        {
            Test.Function(Test.Function2(VeryLongFunction(GenericFunc(Test.Stuff.a), test_global_1, {}, [], ()), 3.f), (float)test_global_2);
        }
    }
    namespace OtherNamespace { }
}

int test_global_1, test_global_2;
{
    int test_local_1, test_local_2;
}

{
    TestA.stuff1 TestB;
    TestB.stuff2 TestA;
    
    int a;
    Test t = new Test
    {
        a = a,
        
        stuff = new Test.Stuff
        {
            a = 0,
            b = .5f,
        },
        
        new Stuff
        {
            DoStuff = 10;
        },
    };
    
    Test.Stuff s = new Test.Stuff
    {
        a = 10,
        b = 5.5f,
    };
    
    Stuff stuff = new Stuff
    {
        a = 10,
        DoStuff = 10,
    }
}

struct MyGenericType<T>
{
    public int a;
    public T b;
    public void DoShit(int someArg) { }
    public T DoGenericShit<U>(T a, U b) { return a; }
    
    /* Test commment 1
    // @tag comment
    */
}

/* Test commment
// TODO: todo comment
*/

MyGenericType<int /**/> myType;

void GenericFunc<T>(int arg)
{
    T a;
    int test_local_1;
    int test_global_1;
    a = test_local_1 + test_global_1;
}

struct TesStruct;
enum TestSingleEnum;

enum EnumTest
{
    A = {0},
    B = 5,
    C = fjldasd
}

class Base { int a; }
class Interface { int b; }
class Derive : Base, Interface { int c; }
class DeriveDerive : Derive;

DeriveDerive.a;
DeriveDerive.c;
Derive a;
a.a;

namespace TestNamespace
{
    int Namespace;
}
namespace TestNamespace.A.B.C
{
    void Namespace();
}
namespace TestNamespace.A;
namespace TestNamespace.A.B { class Namespace; }

//string verbatimStr1 = @"C:\Users\scoleridge\Documents\";
//string verbatimStr2 = @"My pensive SARA ! thy soft cheek reclined
//Thus on mine arm, most soothing sweet it is
//To sit beside our Cot,...";
string verbatimStr3 = @"Her name was ""Sara.""";
string interpolStr = $"{ new StringBuilder(){ } + "Something" } this is string";
string normalStr = "{";

TestNamespace.Namespace = 10;
TestNamespace.A.B.C.Namespace();

#define TEST_MACRO
#if TEST_MACRO
int handsome =
{};
Test /*Some @test comment*/ testDecl1;
testDecl1;
Test.Stuff testDecl2;
Test /*Some other comment*/ CreateTest(int a, int b, float c);
Test.Stuff[][][] VeryLongFunction(Test.Stuff something, int someOtherThing, float fl, Optional[] optionals, params OtherTest[] others,
                                  int[][][] veryLongArg, Entity. A ./*comment*/ EntityAbility anotherLongArg, bool[][][][][][][] values,
                                  Generic<Type<T>> a, SomeOtherType type, System.Type otherType, EnumTest enumTest, TestStruct testStruct,
                                  TestSingleEnum /*comment*/ senum, /*comment*/ AnotherArg arg);
#endif

#if UNKNOW
handsome = 10;
#endif

struct Vector2
{
    public static Vector2 operator +(Vector2 a, Vector2 b);
    public static Vector2 operator -(Vector2 a);
    public static Vector2 operator -(Vector2 a, Vector2 b);
    public static Vector2 operator *(float d, Vector2 a);
    public static Vector2 operator *(Vector2 a, float d);
    public static Vector2 operator *(Vector2 a, Vector2 b);
    public static Vector2 operator /(Vector2 a, float d);
    public static Vector2 operator /(Vector2 a, Vector2 b);
    public static bool operator ==(Vector2 lhs, Vector2 rhs);
    public static bool operator !=(Vector2 lhs, Vector2 rhs);
    
    public static implicit operator Vector2(Vector3 v);
    public static implicit operator Vector3(Vector2 v);
    public static implicit operator Vector3<T>(T t);
    
    public Vector2(float x, float y);
}

Vector2 vector2 = new Vector2(test_global_1, test_global_2);
Vector3.Vector2 vector3 = new Vector3.Vector2(test_global_1, test_global_2);

class Vector3
{
    public void Vector2();
    
    class Vector2
    {
        public Vector2(float x, float y);
        public Vector2<T, U>(T t, U u);
    }
}

public void Vector3();

[
 NewLine(a = { }, b = {0})
 ]
[SomeOtherThing()]
[Multiple] [System.Serializable]
[System.OtherCrap]
public class Test
{
    public const int DoStuff = 5;
    A<B<C>> d;
    Some<Other<Thing<I<Hate>>>> a;
    int a, b, c, d, int f, float h, efldsfjsl, 123, const, int, float a, c;
    public Stuff stuff;
    
    (int, float) tupple;
    public (int, float[][][][]) Function(int someInt, float someFloat)
    {
        
    }
    
    (Generic<Test>, SomeOtherType<Array[]>) Function2(int, float);
    
    public Test()
    {
        
    }
    
    public int property
    {
        get => SomeFunc(_ => int a = 0);
        set =>
        {
            int b = 10;
            SomeOtherFunc((x, y, z) => { });
        }
    };
    
    void Something()
    {
        int a = A;
        System.Action b = (e, f, g) =>
        {
            a = 5;
            b = null;
            {
                int b = 10 * e / f + g;
                DoStuff = b - 5;
            }
        }, c = null;
        
        {
            void Stuff() {}
            Stuff();
            Test.Stuff a;
        }
        
        {
            int DoStuff = 0;
            DoStuff = 10;
        }
        
        {
            int DoStuff = 20;
            DoStuff--;
        }
        
        {
            void DoStuff() { int a, int b; }
            DoStuff();
            Test.Something();
            VeryLongFunction(Function(a, d), SomeOtherThing(Test.DoStuff));
            VeryLongFunction;
        }
        
        {
            Test->Stuff stuff;
            stuff.DoStuff();
        }
        
        {
            void A(int a, int b);
            void B(int a, int b, int c);
            A(B(Something(), {0 + 1} * (3 - 5), Optional.Optional(Test.Stuff.DoStuff())), a);
        }
        
        {
            Test t = new Test();
            t.DoStuff = 10;
            t.Something();
            CreateTest().Something();
        }
        
        if (something == new Vector2(TestIf * ifDecl)) { }
        
        DoStuff = 2;
        Test.DoStuff = 5;
        Stuff->DoStuff();
        OtherTest?.DoStuff();
    }
    
    void SomeOtherThing(int DoStuff)
    {
        Test.DoStuff;
        int a = 0;
        DoStuff = 0;
    }
    
    class Stuff
    {
        int a;
        float b;
        
        void DoStuff()
        {
            void DoStuff()
            {
                DoStuff();
                this.DoStuff();
                Stuff.DoStuff();
                void DoStuff() { };
            }
            DoStuff();
            this.DoStuff();
        }
    }
}

public class OtherTest
{
    void DoStuff() { };
}

class Stuff
{
    static int DoStuff;
}

[
 System.Serializable
 ]
public class Optional<T>
{
    public bool enabled;
    public T value;
    public int some;
    public Something something;
    
    public const int ConstantNumber = 5;
    
    public delegate float SmoothFunc(float a, float b);
    public delegate float SomeOtherDelegate(float[] arr, float* b, Generic<T> c, int d);
    
    public Optional(T initValue)
    {
        enabled = true;
        value = initValue;
    }
    
    public Optional()
    {
        enabled = false;
    }
    
    public static implicit operator T(Optional<T> optional)
    {
        a < b && b > c;
        return optional.value.otherThing;
    }
    
    void SomeFunc1(int a) => { int a = 5, b = 10; }
    void SomeFunc2(int a) => int b = 5;
    A<T>[] GenericFunc(int a) { }
    
    public static Optional DoStuff(Optional[] args)
    {
        DoStuff(args);
        Optional.DoStuff(0);
        Test.DoStuff = 10;
    }
}

public struct Property<T> : ISerializationCallbackReceiver where T : System.Enum
{
    public string[] serializedEnumNames;
    public ulong properties;
    
    private string[] enumNames => System.Enum.GetNames(typeof(T));
    
    [Header("Game Mode")]
    public Property(params T[] properties) : this()
    {
        DoStuff(properties);
        Optional.DoStuff(0);
        string[] names = enumNames;
        GameDebug.Assert(names.Length <= sizeof(ulong) * 8, "Enum: " + names.Length);
        GameDebug.Assert(properties.Length <= names.Length, "Properties: " + properties.Length);
        SetProperties(properties);
    }
    
    void Stuff() { }
    
    public bool[] HasProperty(T property)
    {
        int p = System.Convert.ToInt32(property);
        return (properties & (1ul << p)) != 0;
    }
    
    public void SetProperty(T property, bool set)
    {
        int p = System.Convert.ToInt32(property);
        properties = MathUtils.SetFlag(properties, p, set);
    }
    
    public void SetProperties(params T[] properties)
    {
        if (properties != null)
            foreach (T property in properties)
                SetProperty(property, true);
    }
    
    public override string ToString()
    {
        return "Enum: " + typeof(T) + " Properties: " + properties;
    }
    
    //
    // NOTE: OnBeforeSerialize will get called before the code is compiled and OnAfterDeserialize will get called afterward.
    // The inspector will call OnBeforeSerialize multiple times then call OnGUI but never OnAfterDeserialize.
    public void OnBeforeSerialize() { }
    public void OnAfterDeserialize()
    {
        string[] enumNames = this.enumNames;
        Debug.Assert(enumNames.Length <= sizeof(ulong) * 8, "Enum " + typeof(T).FullName + " has " + enumNames.Length);
        Debug.Assert(enumNames.Length > 0, "Enum " + typeof(T).FullName);
        
        if (serializedEnumNames == null || serializedEnumNames.Length == 0)
        {
            Debug.Log(typeof(T).Name + " is empty");
            serializedEnumNames = enumNames;
        }
        
        bool[] sets = new bool[enumNames.Length];
        for (int i = 0; i < enumNames.Length; i++)
        {
            bool currentValue = MathUtils.HasFlag(properties, i);
            string currentName = enumNames[i];
            if (i >= serializedEnumNames.Length || currentName != serializedEnumNames[i])
            {
                bool findOldIndex = false;
                for (int oldIndex = 0; oldIndex < serializedEnumNames.Length; oldIndex++)
                {
                    if (serializedEnumNames[oldIndex] == currentName)
                    {
                        currentValue = MathUtils.HasFlag(properties, oldIndex);
                        findOldIndex = true;
                        if (currentValue)
                            Debug.Log("New index for " + serializedEnumNames[oldIndex] + " is " + i);
                        break;
                    }
                }
                
                if (!findOldIndex)
                {
                    string msg = "Can't find old value for " + currentName + " at " + i;
                    if (i < serializedEnumNames.Length)
                        msg += ". Old name is " + serializedEnumNames[i];
                    if (currentValue)
                        Debug.LogWarning(msg);
                    else
                        Debug.Log(msg);
                    currentValue = false;
                }
            }
            sets[i] = currentValue;
        }
        
        for (int i = 0; i < sets.Length; i++)
            properties = MathUtils.SetFlag(properties, i, sets[i]);
        
        serializedEnumNames = enumNames;
    }
}

[System.Serializable]
public struct RangedInt
{
    [Tooltip("Min Inclusive")]
    public int min;
    [Tooltip("Max Inclusive")]
    public int max;
    
    public RangedInt(int min, int max)
    {
        this.min = min;
        this.max = max;
    }
    
    public int randomValue => Random.Range(min, max + 1);
}

[System.Serializable]
public struct RangedFloat
{
    [Tooltip("Min Inclusive")]
    public float min;
    [Tooltip("Max Inclusive")]
    public float max;
    
    public RangedFloat(float min, float max)
    {
        this.min = min;
        this.max = max;
    }
    
    public float randomValue
    {
        get
        {
            if (min == max)
                return min;
            return Random.Range(min, max);
        }
    }
    
    public float range => max - min;
}

[System.Serializable]
public class IntReference
{
    public bool useConstant = true;
    public int constantValue;
    public IntVariable variable;
    
    public int value
    {
        get => useConstant ? constantValue : variable.value;
        set
        {
            if (useConstant)
                constantValue = value;
            else
                variable.value = value;
        }
    }
    
    public IntReference(int value)
    {
        useConstant = true;
        constantValue = value;
    }
    
    public static implicit operator int(IntReference reference)
    {
        return reference.value;
    }
}

using System;
using System.Collections.Generic;
using UnityEngine;
using Edgar.Unity;
using UnityEngine.Tilemaps;
using StringBuilder = System.Text.StringBuilder;

public enum GameMode
{
    None,
    Quit,
    Main,
    Play,
    // Cutscene mode
    
    Count
}

[SomethingHere]
public class GameManager : MonoBehaviour
{
    [Header("Game Mode")]
    public GameMode startMode;
    
    public GameObject mainMode;
    public GameObject playMode;
    public LevelData[] levels;
    public int currentLevel;
    
    [Header("Camera")]
    public Vector3Variable playerPos;
    public Entity cameraEntity;
    
    [Header("UI")]
    public bool overrideGameUI;
    public GameMenu gameMenu;
    
    [Header("Other")]
    public Audio[] audios;
    public AudioType firstMusic;
    public int sourceCount;
    public Pool[] pools;
    
    private List<RoomInstance> rooms;
    private int currentRoom;
    
    public static Entity player;
    public static Camera mainCam;
    public static GameUI gameUI;
    
    private static Bounds defaultBounds;
    private static Tilemap tilemap;
    
    [
     NewLine
     ]
    [SomeOtherThing]
    [Multiple] [System.Serializable]
    [System.OtherCrap]
    struct Bounds
    {
        int a; int b;
        int c;
    };
    
#if UNITY_EDITOR
    [EasyButtons.Button]
    private static void FindAllEntityProperties(bool generateFile)
    {
        var watch = new System.Diagnostics.Stopwatch();
        watch.Start();
        StringBuilder builder = new StringBuilder("Date: [" + DateTime.Now.ToString() + "]\n\n", 1024 * 128);
        
        SaveEntityProperties(builder, "Prefab", () =>
                             {
                                 string[] assets = UnityEditor.AssetDatabase.FindAssets("t:Prefab");
                                 List<Entity> entities = new List<Entity>(assets.Length);
                                 foreach (string asset in assets)
                                 {
                                     string path = UnityEditor.AssetDatabase.GUIDToAssetPath(asset);
                                     Entity entity = UnityEditor.AssetDatabase.LoadAssetAtPath<Entity>(path);
                                     if (entity)
                                         entities.Add(entity);
                                 }
                                 return entities;
                             });
        
        SaveEntityProperties(builder, "Object", () =>
                             {
                                 Entity[] assets = FindObjectsOfType<Entity>(true);
                                 List<Entity> entities = new List<Entity>(assets.Length + 1);
                                 entities.AddRange(assets);
                                 return entities;
                             });
        
        string message = "Search complete";
        UnityEngine.Object context = null;
        if (generateFile)
        {
            string path = GameUtils.CreateUniquePath("Assets/Files/properties.txt");
            System.IO.File.WriteAllText(path, builder.ToString());
            UnityEditor.AssetDatabase.SaveAssets();
            UnityEditor.AssetDatabase.Refresh();
            message = "File is created at " + path;
            context = UnityEditor.AssetDatabase.LoadAssetAtPath<TextAsset>(path);
        }
        watch.Stop();
        Debug.Log(message + $" after {watch.ElapsedMilliseconds}ms", context);
        
        static void SaveEntityProperties(StringBuilder builder, string title, Func<List<Entity>> getEntities)
        {
            List<Entity> entities = getEntities();
            entities.Insert(0, null);
            
            builder.Append("\n");
            foreach (Entity entity in entities)
            {
                int startIndex = builder.Length;
                if (entity == null)
                    builder.Append($"-------- -------- -------- -------- {{{title}}}: {entities.Count - 1,4} -------- -------- -------- --------");
                SerializeType(entity, builder, $"-- {entity?.name} ({entity?.GetInstanceID()}) --");
                Debug.Log(builder.ToString(startIndex, builder.Length - startIndex), entity?.gameObject);
                builder.Append("\n");
            }
        }
        
        static void PrintProperty(object obj, string fieldName, StringBuilder builder, int indentLevel)
        {
            ulong property = GameUtils.GetValueFromObject<ulong>(obj, "properties");
            string[] names = GameUtils.GetValueFromObject<string[]>(obj, "serializedEnumNames", true);
            
            builder.AppendIndentLine(fieldName, indentLevel);
            builder.AppendIndentFormat("Properties of {0}: ", indentLevel + 1, fieldName).Append(property.ToString()).Append(", ")
                .AppendLine(Convert.ToString((long)property, 2));
            
            List<string> setNames = new List<string>(names.Length);
            for (int i = 0; i < names.Length; i++)
                if (MathUtils.HasFlag(property, i))
                    setNames.Add(names[i]);
            GameUtils.GetAllString(setNames, $"All set flags of {fieldName}: ", "\n", indentLevel + 1, builder: builder);
            
            GameUtils.GetAllString(names, $"Serialized names of {fieldName}: ", "\n", indentLevel + 1,
                                   toString: (name, i) => name + ": " + (MathUtils.HasFlag(property, i) ? "1" : "0"), builder: builder);
        }
        
        // NOTE: This doesn't work if the obj is already an Property
        static void SerializeType(object obj, StringBuilder builder, string fieldName)
        {
            GameUtils.SerializeType(new object[] { obj },
                                    data => (data.type?.IsGenericType ?? false) && (data.type?.GetGenericTypeDefinition() == typeof(Property<>)), data =>
                                    {
                                        object property = data.objs[0];
                                        string name = fieldName;
                                        
                                        if (data.parent?.parent != null)
                                        {
                                            // TODO: I only handle the case when the current or parent is an element from an array. Hanlde the remaining cases.
                                            bool isArrayElement = data.parent.type.IsArray;
                                            bool isParentArrayElement = data.parent.parent.type.IsArray;
                                            // TODO: Handle array of serializable type contains array of serializable type contains array of...
                                            Debug.Assert(!(isArrayElement && isParentArrayElement));
                                            
                                            if (isParentArrayElement)
                                                data = data.parent;
                                            name = data.parent.field.Name;
                                            if (isArrayElement || isParentArrayElement)
                                                name += $"[{data.index}]";
                                        }
                                        PrintProperty(property, name, builder, data.parent.depth);
                                    }, data => data.type != null && !GameUtils.IsSerializableType(data.type), (data, recursive) => recursive());
        }
    }
#endif
    
    // RANT: This function only gets called in LevelInfoPostProcess.cs and its job is to:
    // 1. Add 2 rule tiles at the 2 ends of a door line.
    // 2. Delete all the tiles that are at the door's tiles.
    // 3. Delete all the tiles that are next to the door's tiles.
    // Originally, I let Edgar's corridor system to handle the first 2 cases (I have to have a shared tilemap).
    // Now, I don't need the corridor system anymore but I still need a shared tilemap.
    // Obviously, these cases can be easily solved if Unity's rule tile system let you work with out-of-bounds cases, but it doesn't :(
    // If Unity ever allow that then I can remove most of this code and also don't need a shared tilemap anymore.
    public void InitTilemap(List<RoomInstance> rooms, Tilemap tilemap, TileBase ruleTile)
    {
        foreach (var room in rooms)
        {
            foreach (var door in room.Doors)
            {
                if (door.ConnectedRoomInstance != null)
                {
                    Vector3Int dir = door.DoorLine.GetDirectionVector();
                    tilemap.SetTile(door.DoorLine.From + room.Position - dir, ruleTile);
                    tilemap.SetTile(door.DoorLine.To + room.Position + dir, ruleTile);
                    
                    foreach (Vector3Int doorTile in door.DoorLine.GetPoints())
                    {
                        Vector3Int pos = doorTile + room.Position;
                        tilemap.SetTile(pos, null);
                        Remove(tilemap, pos, -(Vector3Int)door.FacingDirection);
                        
                        static void Remove(Tilemap tilemap, Vector3Int pos, Vector3Int removeDir)
                        {
                            pos += removeDir;
                            if (tilemap.GetTile(pos))
                            {
                                tilemap.SetTile(pos, null);
                                Remove(tilemap, pos, removeDir);
                            }
                        }
                    }
                }
            }
        }
        
        tilemap.CompressAndRefresh();
        this.rooms = rooms;
        GameManager.tilemap = tilemap;
        Debug.Log(tilemap);
    }
    
    public static Tilemap GetTilemapFromRoom(Transform roomTransform)
    {
        return roomTransform.GetChild(0).GetChild(2).GetComponent<Tilemap>();
    }
    
    public static Bounds GetBoundsFromTilemap(Tilemap tilemap)
    {
        Bounds bounds = tilemap.cellBounds.ToBounds();
        bounds.center += tilemap.transform.position;
        return bounds;
    }
    
    public static Bounds GetBoundsFromRoom(Transform roomTransform)
    {
        return roomTransform != null ? GetBoundsFromTilemap(GetTilemapFromRoom(roomTransform)) : defaultBounds;
    }
    
    public static bool GetGroundPos(Vector2 pos, Vector2 extents, float dirY, out Vector3Int groundPos, out Vector3Int emptyPos)
    {
        RaycastHit2D hitInfo = GameUtils.GroundCheck(pos, extents, dirY, Color.cyan);
        groundPos = Vector3Int.zero;
        emptyPos  = Vector3Int.zero;
        if (hitInfo)
        {
            Vector3Int hitPosCeil  = QueryTiles(tilemap, hitInfo.point.ToVector2Int(true ).ToVector3Int(), true, 0, (int)dirY);
            Vector3Int hitPosFloor = QueryTiles(tilemap, hitInfo.point.ToVector2Int(false).ToVector3Int(), true, 0, (int)dirY);
            groundPos = Mathf.Abs(hitPosFloor.y - hitInfo.point.y) < Mathf.Abs(hitPosCeil.y - hitInfo.point.y) ? hitPosFloor : hitPosCeil;
            emptyPos  = groundPos + new Vector3Int(0, -(int)dirY, 0);
        }
        return hitInfo;
    }
    
    public static Rect CalculateMoveRegion(Vector2 pos, Vector2 extents, float dirY)
    {
        Rect result = new Rect();
        Debug.Assert(dirY == Mathf.Sign(dirY), dirY);
        if (GetGroundPos(pos, extents, dirY, out Vector3Int hitPos, notDecl, out Vector3Int empPos))
        {
            Vector3Int minGroundPos = QueryTiles(tilemap, hitPos, false, -1, 0);
            Vector3Int maxGroundPos = QueryTiles(tilemap, hitPos, false, +1, 0);
            
            Vector3Int minWallPos   = QueryTiles(tilemap, empPos, true , -1, 0);
            Vector3Int maxWallPos   = QueryTiles(tilemap, empPos, true , +1, 0);
            
            if (dirY < 0)
            {
                minGroundPos.y++;
                maxGroundPos.y++;
            }
            else if (dirY > 0)
            {
                minWallPos.y++;
                maxWallPos.y++;
            }
            
            Vector2 minPos = MathUtils.Max((Vector2Int)minGroundPos, (Vector2Int)minWallPos) + new Vector2(1, 0);
            Vector2 maxPos = MathUtils.Min((Vector2Int)maxGroundPos, (Vector2Int)maxWallPos);
            result = MathUtils.CreateRectMinMax(minPos + extents * new Vector2(+1, -dirY), maxPos + extents * new Vector2(-1, -dirY));
            
            //GameDebug.Log($"Hit pos: {hitPos}, Min pos: {minPos}, Max pos: {maxPos}, Rect: {result}");
            GameDebug.DrawBox(result, Color.green);
            GameDebug.DrawLine((Vector3)minGroundPos, (Vector3)maxGroundPos, Color.red);
            GameDebug.DrawLine((Vector3)minWallPos, (Vector3)maxWallPos, Color.yellow);
        }
        return result;
    }
    
    private static Vector3Int QueryTiles(Tilemap tilemap, Vector3Int startPos, bool breakCondition, int advanceX, int advanceY)
    {
        Vector3Int pos = startPos;
        while (tilemap.GetTile(pos) != breakCondition)
            pos += new Vector3Int(advanceX, advanceY, 0);
        return pos;
    }
    
    private void Start()
    {
        ObjectPooler.Init(gameObject, pools);
        AudioManager.Init(gameObject, audios, firstMusic, sourceCount);
        gameUI = FindObjectOfType<GameUI>();
        mainCam = Camera.main;
        cameraEntity = mainCam.GetComponentInParent<Entity>();
        StartGameMode(startMode);
        
        // Camera
        GameDebug.BindInput(new DebugInput
                            {
                                trigger = InputType.Debug_CameraShake, increase = InputType.Debug_ShakeIncrease, decrease = InputType.Debug_ShakeDecrease,
                                range = new RangedFloat(0, Enum.GetValues(typeof(ShakeMode)).Length - 1),
                                updateValue = (value, dir) => value + dir,
                                callback = mode => { GameDebug.Log((ShakeMode)mode); CameraSystem.instance.Shake((ShakeMode)mode, MathUtils.SmoothStart3); },
                            });
        GameDebug.BindInput(InputType.Debug_CameraShock, () => CameraSystem.instance.Shock(2));
        
        // Log
        GameDebug.BindInput(InputType.Debug_ToggleLog, () => GameDebug.ToggleLogger());
        GameDebug.BindInput(InputType.Debug_ClearLog, () => GameDebug.ClearLog());
        
        // Time
        GameDebug.BindInput(new DebugInput
                            {
                                trigger = InputType.Debug_ResetTime, increase = InputType.Debug_FastTime, decrease = InputType.Debug_SlowTime,
                                value = 1f, range = new RangedFloat(.125f, 4f),
                                updateValue = (value, dir) => value * Mathf.Pow(2, dir),
                                changed = scale => { Time.timeScale = scale; GameDebug.Log(Time.timeScale); },
                                callback = _ => { Time.timeScale = 1; GameDebug.Log("Reset Time.timeScale!"); }
                            });
        
        // VFX
        GameDebug.BindInput(InputType.Debug_SpawnParticle, () => ParticleEffect.instance.SpawnParticle(ParticleType.Explosion, Vector2.zero, 1));
        GameDebug.BindInput(InputType.Debug_TestPlayerVFX, player.TestPlayerVFX);
    }
    
    private void Update()
    {
        GameDebug.UpdateInput();
        
        if (rooms != null)
        {
            if (levels[currentLevel].moveAutomatically)
            {
                if (cameraEntity.CompleteCycle() && currentRoom < rooms.Count)
                {
                    float aspectRatio = 16f / 9f;
                    Transform roomTransform = rooms[currentRoom++].RoomTemplateInstance.transform;
                    Bounds bounds = GetBoundsFromRoom(roomTransform);
                    float ratio = bounds.extents.x / bounds.extents.y;
                    mainCam.orthographicSize = ratio <= aspectRatio ? (bounds.extents.x / aspectRatio) : bounds.extents.y;
                    GameInput.TriggerEvent(GameEventType.NextRoom, roomTransform);
                }
            }
            else
            {
                for (int i = 0; i < rooms.Count; ++i)
                {
                    if (i != currentRoom)
                    {
                        // NOTE: This will make sure the player has already moved pass the doors
                        Rect roomRect = GetBoundsFromRoom(rooms[i].RoomTemplateInstance.transform).ToRect().Resize(Vector2.one * 2, Vector2.one * -2);
                        if (roomRect.Contains(player.transform.position))
                        {
                            GameInput.TriggerEvent(GameEventType.NextRoom, rooms[i].RoomTemplateInstance.transform);
                            currentRoom = i;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    public void StartGameMode(GameMode mode)
    {
        if (mode == GameMode.None || mode == GameMode.Count)
            return;
        if (mode == GameMode.Quit)
        {
            Application.Quit();
            return;
        }
        
        // Reset the game
        {
            // TODO:
            //  - Reset all the scriptable objects
            //  - Reset all game input's events
            //  - Reset persistent entities like camera
            rooms?.Clear();
            gameMenu.gameObject.SetActive(false);
            
            bool isMainMode = mode == GameMode.Main;
            // NOTE: I didn't use the ?. operator because Unity's GameObject has a custom == operator but doesn't have a custom ?. operator
            if (mainMode) mainMode.SetActive(isMainMode);
            playMode.SetActive(!isMainMode);
        }
        
        switch (mode)
        {
            case GameMode.Main:
            {
                AudioManager.PlayAudio(AudioType.Music_Main);
            } break;
            case GameMode.Play:
            {
                {
                    for (int i = 0; i < levels.Length; i++)
                        levels[i].gameObject.SetActive(i == currentLevel);
                    if (overrideGameUI)
                    {
                        gameUI.displayHealthBar = gameUI.displayMoneyCount = gameUI.displayWaveCount = gameUI.displayWeaponUI = true;
                        gameUI.enabled = levels[currentLevel].enableUI;
                        gameUI.displayMinimap = levels[currentLevel].enableMinimap;
                    }
                }
                
                LevelData level = levels[currentLevel];
                cameraEntity.properties.SetProperty(EntityProperty.CustomInit, true);
                cameraEntity.CustomInit();
                cameraEntity.InitCamera(level.moveAutomatically, level.useSmoothDamp, level.cameraValue, level.waitTime);
                
                switch (level.type)
                {
                    case BoundsType.Generator:
                    {
                        {
                            bool levelGenerated = false;
                            int count = level.maxGenerateTry;
                            DungeonGenerator generator = level.generator;
                            generator.transform.Clear();
                            
                            while (!levelGenerated)
                            {
                                try
                                {
                                    if (count == 0)
                                    {
                                        Debug.LogError("Level couldn't be generated!");
                                        break;
                                    }
                                    generator.Generate();
                                    levelGenerated = true;
                                }
                                catch (InvalidOperationException)
                                {
                                    Debug.LogError("Timeout encountered");
                                    count--;
                                }
                            }
                        }
                        
                        if (!level.moveAutomatically)
                        {
                            currentRoom = -1;
                            GameInput.BindEvent(GameEventType.EndRoom, room => LockRoom(room, false, false));
                            GameInput.BindEvent(GameEventType.NextRoom, room => LockRoom(room, true, level.disableEnemies));
                        }
                        else
                        {
                            currentRoom = 0;
                            PlayerController player = FindObjectOfType<PlayerController>();
                            if (player)
                                Destroy(player.gameObject);
                        }
                    } break;
                    case BoundsType.Tilemap:
                    {
                        tilemap = level.tilemap.CompressAndRefresh();
                        defaultBounds = level.tilemap.cellBounds.ToBounds();
                        GameInput.TriggerEvent(GameEventType.NextRoom, null);
                    } break;
                    case BoundsType.Custom:
                    {
                        defaultBounds = new Bounds(Vector3.zero, level.boundsSize + mainCam.HalfSize() * 2);
                        GameInput.TriggerEvent(GameEventType.NextRoom, null);
                    } break;
                }
            } break;
        }
        
        player = GameObject.FindGameObjectWithTag("Player")?.GetComponent<Entity>();
    }
    
    static void LockRoom(Transform room, bool lockRoom, bool disableEnemies)
    {
        if (lockRoom && !disableEnemies)
        {
            EnemySpawner spawner = room?.GetComponentInChildren<EnemySpawner>(true);
            if (!spawner)
                return;
            spawner.enabled = true;
        }
        
        Transform doorHolder = room?.Find("Doors");
        if (doorHolder)
        {
            if (lockRoom)
                doorHolder.gameObject.SetActive(true);
            else
                Destroy(doorHolder.gameObject, 1f);
            
            foreach (Transform door in doorHolder)
            {
                Animator doorAnim = door.GetComponent<Animator>();
                doorAnim.Play(lockRoom ? "Lock" : "Unlock");
            }
        }
    }
    
    private void OnAudioFilterRead(float[] data, int channels) => AudioManager.ReadAudio(data, channels);
}

#define SCRIPTABLE_VFX
using System.Collections;
using UnityEngine;

public enum EntityProperty
{
    CanJump,
    CanBeHurt,
    DamageWhenCollide,
    DieWhenCollide,
    DieAfterMoveTime,
    SpawnCellWhenDie,
    SpawnDamagePopup,
    ClampToMoveRegion,
    StartAtMinMoveRegion,
    IsCritical,
    UsePooling,
    CustomInit, // TODO: Maybe collapse this and UsePooling into one
    AddMoneyWhenCollide,
    FallingOnSpawn,
    
    // Don't show in the inspector
    IsGrounded,
    IsReloading,
    AtEndOfMoveRegion,
    
    Count
}

public enum EntityState
{
    None,
    
    Jumping,
    Falling,
    Landing,
    
    StartMoving,
    StopMoving,
    
    StartAttack,
    StartCooldown,
    
    OnSpawn,
    OnHit,
    OnDeath,
}

[SerializedPool]
public class Entity : MonoBehaviour, IPooledObject
{
#region Ability
    /*
    * RUNNING
    * - Turn speed
    * - Acceleration
    * - Decceleration
    * - Max speed
    * JUMPING
    * - Duration
    * - Jump height
    * - Down gravity
    * - Air acceleration (what about air decceleration?)
    * - Air control (movement in air/can the player change direction in air?/The equivalent of turn speed but in air)
    * - Air brake (does the player still move forward when he stop pressing?)
    * CAMERA
    * - Damping (X/Y/Jump)
    * - Lookahead
    * - Zoom
    * ASSISTS
    * - Coyote time
    * - Jump buffer
    * - Terminal velocity
    * - Rounded corners
    * JUICE
    * - Particles (run/jump/land)
    * - Squash and stretch (jump/land)
    * - Trail
    * - Lean (angle and speed)
    */
    
    // Move horizontally based on input
    // Jump
    // Flip gravity based on input
    // dash when near
    // explode when near/died
    // Teleport when player is far away
    // Move towards the player, teleport when hit a cliff
    
    public enum AbilityType
    {
        None,
        Move,
        Teleport,
        Explode,
        Jump,
    }
    
    public enum AbilityFlag
    {
        Interuptible,
        AwayFromPlayer,
        LockOnPlayerForEternity,
        
        ExecuteWhenLowHealth,
        ExecuteWhenInRange,
        ExecuteWhenInRangeY,
        ExecuteWhenOutOfMoveRegion,
        OrCombine,
        
        CanExecute,
    }
    
    [System.Serializable]
    public class EntityAbility
    {
        public Property<AbilityFlag> flags;
        public EntityVFX vfx;
        public AbilityType type;
        
        public int healthToExecute;
        public float distanceToExecute;
        public float distanceToExecuteY;
        public float cooldownTime;
        public float interuptibleTime;
        
        public float chargeTime;
        public float duration;
        public float range;
        public int damage;
    }
    
    public bool CanUseAbility(EntityAbility ability)
    {
        Property<AbilityFlag> flags = ability.flags;
        Vector2 pos = GameManager.player?.transform.position ?? (transform.position + Vector3.one);
        
        bool lowHealth  = flags.HasProperty(AbilityFlag.ExecuteWhenLowHealth)       == (health < ability.healthToExecute);
        bool isInRange  = flags.HasProperty(AbilityFlag.ExecuteWhenInRange)         == IsInRange(ability.distanceToExecute, pos);
        bool isInRangeY = flags.HasProperty(AbilityFlag.ExecuteWhenInRangeY)        == IsInRangeY(ability.distanceToExecuteY, pos);
        bool moveRegion = flags.HasProperty(AbilityFlag.ExecuteWhenOutOfMoveRegion) == HasProperty(EntityProperty.AtEndOfMoveRegion);
        
        bool result = Check(ability.flags.HasProperty(AbilityFlag.OrCombine), lowHealth, isInRange, isInRangeY, moveRegion);
        return result;
        
        static bool Check(bool condition, params bool[] values)
        {
            foreach (bool b in values)
                if (b == condition)
                    return condition;
            return !condition;
        }
    }
    
    public IEnumerator UseAbility(EntityAbility ability, MoveType moveType, TargetType targetType, float speed)
    {
        ability.flags.SetProperty(AbilityFlag.CanExecute, false);
        float cooldownTime = 0;
        this.moveType = MoveType.None;
        float timer = 0;
        ability.vfx.canStop = () => ability.flags.HasProperty(AbilityFlag.CanExecute);
        PlayVFX(ability.vfx);
        while (timer < ability.interuptibleTime)
        {
            if (ability.flags.HasProperty(AbilityFlag.Interuptible))
                if (!CanUseAbility(ability))
                    goto END;
            yield return null;
            timer += Time.deltaTime;
        }
        
        this.targetType = TargetType.None;
        yield return new WaitForSeconds(ability.chargeTime - ability.interuptibleTime);
        
        switch (ability.type)
        {
            case AbilityType.Move:
            {
                this.moveType = moveType;
                this.speed = ability.range / ability.duration;
                this.targetType = targetType;
                damage = ability.damage;
                if (ability.flags.HasProperty(AbilityFlag.AwayFromPlayer))
                    targetDir = MathUtils.Sign(targetPos - (Vector2)transform.position) * new Vector2(-1, 1);
                if (ability.flags.HasProperty(AbilityFlag.LockOnPlayerForEternity))
                {
                    testProperties[0].SetProperty(VFXProperty.StartTrailing, false);
                    StartFalling(false);
                    yield break;
                }
            } break;
            case AbilityType.Teleport:
            {
                float playerUp = GameManager.player.transform.up.y;
                Vector3 destination;
                {
                    destination = GameManager.player.transform.position;
                    destination.y += (spriteExtents.y - GameManager.player.spriteExtents.y) * playerUp;
                    float distance = ability.range * Mathf.Sign(GameManager.player.transform.position.x - transform.position.x);
                    
                    // TODO: Maybe teleport opposite to where the player is heading or teleport to nearby platform
                    if (!IsPosValid(distance))
                        if (!IsPosValid(-distance))
                            goto END;
                    
                    bool IsPosValid(float offsetX)
                    {
                        Vector3 offset = new Vector3(offsetX, 0);
                        bool onGround = GameUtils.BoxCast(destination + offset - new Vector3(0, spriteExtents.y * playerUp),
                                                          new Vector2(spriteExtents.x, .1f), Color.yellow);
                        bool insideWall = GameUtils.BoxCast(destination + offset + new Vector3(0, .1f * playerUp), sr.bounds.size, Color.green);
                        if (onGround && !insideWall)
                        {
                            destination.x += offsetX;
                            return true;
                        }
                        return false;
                    }
                }
                
                if (playerUp != transform.up.y)
                    transform.Rotate(180, 0, 0);
                
                yield return null;
                transform.position = destination;
                CalculateMoveRegion();
            }
            break;
            case AbilityType.Explode:
            {
                // NOTE: If the enemy die then it will be handled by the vfx system
                if (IsInRange(ability.range, GameManager.player.transform.position))
                    GameManager.player.Hurt(ability.damage);
            } break;
            case AbilityType.Jump:
            {
                
            } break;
        }
        
        yield return new WaitForSeconds(ability.duration);
        cooldownTime = ability.cooldownTime;
        
        END:
        this.speed = speed;
        this.targetType = targetType;
        this.moveType = moveType;
        
        yield return null;
        currentAbility = MathUtils.LoopIndex(currentAbility + 1, abilities.Length, true);
        
        yield return new WaitForSeconds(cooldownTime);
        ability.flags.SetProperty(AbilityFlag.CanExecute, true);
    }
#endregion
    
    public enum TargetOffsetType
    {
        None,
        Mouse,
        Player,
    }
    
    public enum MoveRegionType
    {
        None,
        Ground,
        Vertical,
    }
    
    public enum AttackTrigger
    {
        None,
        MouseInput,
    }
    
    [Header("Stat")]
    public Property<EntityProperty> properties;
    [SerializedPool] public Property<VFXProperty>[] testProperties;
    [SerializedPool] public EntityAbility[] abilities;
    private int currentAbility;
    
    public int health;
    public int damage;
    public int money;
    public int ammo;
    
    public string[] collisionTags;
    [MinMax(0, 10)] public RangedInt valueRange;
    
    [Header("Attack")]
    public WeaponStat stat;
    public AttackTrigger attackTrigger;
    private RangedFloat attackDuration;
    
    [Header("Movement")]
    public MoveType moveType;
    public RotateType rotateType;
    public TargetType targetType;
    [Range(0f, 50f)] public float speed;
    public float dRotate;
    public float range;
    public float maxFallingSpeed;
    public TargetOffsetType offsetType;
    public Vector2 targetOffset;
    public SpringData spring;
    [MinMax(-1, 1)] public RangedFloat fallDir;
    
    public MoveRegionType regionType;
    [ShowWhen("regionType", MoveRegionType.Vertical)] public float verticalHeight;
    private Rect moveRegion;
    
    public float moveTime;
    private float moveTimeValue;
    
    public float groundRememberTime;
    private float groundRemember;
    
    public float fallRememberTime;
    private float fallRemember;
    
    public float jumpPressedRememberTime;
    private float jumpPressedRemember;
    
    public AudioType footstepAudio;
    public RangedFloat timeBtwFootsteps;
    private float timeBtwFootstepsValue;
    
    private Vector2 velocity;
    private Rigidbody2D rb;
    private Collider2D cd;
    private float speedY;
    private Vector2 targetDir;
    private Vector2 targetPos;
    private Vector2 offsetDir;
    private EntityState state;
    
    [Header("Effects")]
    public Material whiteMat;
    public ParticleSystem leftDust;
    public ParticleSystem rightDust;
    public VFXCollection vfx;
    public EntityVFX spawnVFX, deathVFX, hurtVFX;
    
    private TMPro.TextMeshPro text;
    private TrailRenderer trail;
    private Animator anim;
    private SpriteRenderer sr;
    private Vector2 spriteExtents;
    
    public void OnObjectInit()
    {
        if (HasProperty(EntityProperty.UsePooling))
        {
            Init();
            deathVFX.done += () => gameObject.SetActive(false);
        }
    }
    
    public void CustomInit()
    {
        Init();
        OnObjectSpawn();
        // TODO: Figure out whether we need to disable or destroy the object
    }
    
    public void OnObjectSpawn()
    {
        moveTimeValue = moveTime + Time.time;
        CalculateMoveRegion();
        if (HasProperty(EntityProperty.FallingOnSpawn))
            StartFalling(true);
    }
    
    // Start is called before the first frame update
    void Start()
    {
        if (!HasProperty(EntityProperty.UsePooling) && !HasProperty(EntityProperty.CustomInit))
        {
            Init();
            OnObjectSpawn();
            deathVFX.done += () => Destroy(this);
        }
    }
    
#region Initialize
    void Init()
    {
        rb = GetComponent<Rigidbody2D>();
        cd = GetComponent<Collider2D>();
        text = GetComponent<TMPro.TextMeshPro>();
        trail = GetComponent<TrailRenderer>();
        anim = GetComponent<Animator>();
        sr = GetComponent<SpriteRenderer>();
        
        if (rb && maxFallingSpeed != 0)
        {
            float drag = MathUtils.GetDragFromAcceleration(Mathf.Abs(Physics2D.gravity.y * rb.gravityScale), maxFallingSpeed);
            Debug.Assert(drag > 0, drag);
            rb.drag = drag;
        }
        if (whiteMat)
            whiteMat = Instantiate(whiteMat);
        if (sr)
            spriteExtents = sr.bounds.extents;
        
        if (HasProperty(EntityProperty.SpawnCellWhenDie))
            deathVFX.done += () =>
            {
                int dropValue = valueRange.randomValue;
                for (int i = 0; i < dropValue; i++)
                    ObjectPooler.Spawn(PoolType.Cell, transform.position);
            };
        
        MoveType move = moveType;
        RotateType rotate = rotateType;
        hurtVFX.done += () =>
        {
            SetProperty(EntityProperty.CanBeHurt, true);
            moveType = move;
            rotateType = rotate;
        };
        
        ammo = stat?.ammo ?? 0;
        if (spring != null && spring.f != 0)
            spring.Init(GameManager.player.transform.position.y);
        
        state = EntityState.OnSpawn;
    }
    
    public void InitCamera(bool automatic, bool useSmoothDamp, Vector2 value, float waitTime)
    {
        //SetProperty(EntityProperty.HasTargetOffset, !automatic);
        SetProperty(EntityProperty.StartAtMinMoveRegion, automatic);
        offsetType = automatic ? TargetOffsetType.None : TargetOffsetType.Mouse;
        targetType = automatic ? TargetType.MoveRegion : TargetType.Player;
        
        speed = value.magnitude;
        moveType = useSmoothDamp ? MoveType.SmoothDamp : MoveType.Fly;
        speed = value.x;
        speedY = value.y;
        
        // NOTE: This ability only executes when the camera has TargetType.MoveRegion
        abilities[0].flags = new Property<AbilityFlag>(AbilityFlag.ExecuteWhenOutOfMoveRegion, AbilityFlag.CanExecute);
        abilities[0].chargeTime = waitTime;
        
        GameInput.BindEvent(GameEventType.NextRoom, room => ToNextRoom(GameManager.GetBoundsFromRoom(room).ToRect()));
        void ToNextRoom(Rect roomRect)
        {
            moveRegion = roomRect;
            moveRegion.min += GameManager.mainCam.HalfSize();
            moveRegion.max -= GameManager.mainCam.HalfSize();
            Debug.Assert((moveRegion.xMin <= moveRegion.xMax) && (moveRegion.yMin <= moveRegion.yMax),
                         $"Camera's limit is wrong: (Move region: {moveRegion}, Rect: {roomRect})");
        }
    }
    
    public void InitBullet(int damage, bool isCritical, bool hitPlayer)
    {
        this.damage = damage;
        SetProperty(EntityProperty.IsCritical, isCritical);
        collisionTags[1] = hitPlayer ? "Player" : "Enemy";
    }
    
    public void InitDamagePopup(int damage, bool isCritical)
    {
        targetDir = Vector2.one;
        text.text = damage.ToString();
        spawnVFX = new EntityVFX
        {
            properties = new Property<VFXProperty>(VFXProperty.ScaleOverTime, VFXProperty.FadeTextWhenDone),
            scaleTime = moveTime / 2, scaleOffset = Vector2.one / 2, fadeTime = 1f / 3f,
            fontSize = isCritical ? 3f : 2.5f, textColor = isCritical ? Color.red : Color.white,
        };
        PlayVFX(spawnVFX);
    }
    
    [EasyButtons.Button]
    public void Pickup()
    {
        if (GameManager.player)
            spring.Init(GameManager.player.transform.position.y);
        offsetType = TargetOffsetType.Player;
        moveType = MoveType.Spring;
        targetType = TargetType.Player;
        rotateType = RotateType.Weapon;
        attackTrigger = AttackTrigger.MouseInput;
    }
    
    public void Shoot(bool isCritical)
    {
        Entity bullet = ObjectPooler.Spawn<Entity>(PoolType.Bullet_Normal, transform.position, transform.eulerAngles);
        bullet.InitBullet(isCritical ? stat.damage : stat.critDamage, isCritical, false);
    }
#endregion
    
    // Update is called once per frame
    void Update()
    {
        // TODO: Test if this is actually working
        if (Time.deltaTime == 0)
            return;
        
        bool wasGrounded = HasProperty(EntityProperty.IsGrounded);
        // NOTE: We're passing -transform.up.y rather than -rb.gravityScale because:
        // 1. Some objects don't have a rigidbody. It's also in my roadmap to replace the rigidbody system entirely.
        // 2. The isGrounded only equals false if and only if the down velocity isn't zero.
        bool isGrounded = SetProperty(EntityProperty.IsGrounded, GameUtils.GroundCheck(transform.position, spriteExtents, -transform.up.y, Color.red));
        
        if (abilities.Length > 0 && abilities[currentAbility].flags.HasProperty(AbilityFlag.CanExecute) && CanUseAbility(abilities[currentAbility]))
            StartCoroutine(UseAbility(abilities[currentAbility], moveType, targetType, speed));
        
        groundRemember -= Time.deltaTime;
        if (isGrounded)
            groundRemember = groundRememberTime;
        
        fallRemember -= Time.deltaTime;
        if (!isGrounded)
            fallRemember = fallRememberTime;
        
        jumpPressedRemember -= Time.deltaTime;
        if (HasProperty(EntityProperty.CanJump) && GameInput.GetInput(InputType.Jump))
            jumpPressedRemember = jumpPressedRememberTime;
        
        if (HasProperty(EntityProperty.FallingOnSpawn) && !isGrounded)
            return;
        
        {
            bool canShoot = false;
            bool canReload = false;
            switch (attackTrigger)
            {
                case AttackTrigger.MouseInput:
                {
                    if (!HasProperty(EntityProperty.IsReloading))
                    {
                        canShoot  = ammo > 0         && GameInput.GetInput(InputType.Shoot);
                        canReload = ammo < stat.ammo && GameInput.GetInput(InputType.Reload);
                    }
                } break;
            }
            
            if (canShoot && Time.time > attackDuration.max)
            {
                ammo--;
                attackDuration.max = Time.time + stat.timeBtwShots;
                state = EntityState.StartAttack;
                
                bool isCritical = Random.value < stat.critChance;
                float rot = attackDuration.range > 0 ? (Mathf.PerlinNoise(0, attackDuration.range) * 2f - 1f) * 15f : 0;
                Shoot(isCritical);
            }
            else if ((ammo == 0 && canShoot) || canReload)
            {
                StartCoroutine(Reloading(stat, GameManager.gameUI.UpdateReload,
                                         enable =>
                                         {
                                             GameManager.gameUI.EnableReload(enable, stat.standardReload);
                                             GameInput.EnableInput(InputType.Interact, !enable);
                                             SetProperty(EntityProperty.IsReloading, enable);
                                             ammo = enable ? 0 : stat.ammo;
                                         }));
                
                IEnumerator Reloading(WeaponStat stat, System.Func<float, bool, bool> updateUI, System.Action<bool> enable)
                {
                    enable(true);
                    yield return null;
                    
                    float maxTime = stat.standardReload;
                    float t = 0;
                    bool hasReloaded = false;
                    while (t <= maxTime)
                    {
                        yield return null;
                        t += Time.deltaTime;
                        if (!hasReloaded)
                        {
                            bool isPerfect = updateUI(t, hasReloaded = GameInput.GetInput(InputType.Reload));
                            if (hasReloaded)
                                maxTime = isPerfect ? stat.perfectReload : stat.failedReload;
                        }
                    }
                    
                    enable(false);
                }
            }
            else if (!canShoot)
                attackDuration.min = attackDuration.max;
        }
        
        if (Time.time > moveTimeValue)
            if (HasProperty(EntityProperty.DieAfterMoveTime))
                Die();
        
        RotateEntity(transform, rotateType, dRotate, velocity.x);
        Vector2 prevVelocity = velocity;
        MoveEntity();
        
        if (HasProperty(EntityProperty.ClampToMoveRegion))
            transform.position = MathUtils.Clamp(transform.position, moveRegion.min, moveRegion.max, transform.position.z);
        //GameDebug.DrawBox(moveRegion, Color.green);
        
        
        bool startJumping = jumpPressedRemember >= 0 && groundRemember >= 0;
        {
            EntityVFX playerVFX = new EntityVFX
            {
                shakeMode = ShakeMode.PlayerJump,
                waitTime = .25f,
                particles = new ParticleSystem[] { velocity.x >= 0 ? leftDust : null, velocity.x <= 0 ? rightDust : null },
            };
            // Start jumping
            if (startJumping)
            {
                state = EntityState.Jumping;
                jumpPressedRemember = 0;
                groundRemember = 0;
                SetProperty(EntityProperty.IsGrounded, false);
                rb.gravityScale *= -1;
                
                {
                    playerVFX.audio = AudioType.Player_Jump;
                    playerVFX.scaleOffset = new Vector2(-.25f, .25f);
                    playerVFX.rotateTime = .2f;
                    playerVFX.properties.SetProperty(VFXProperty.FlipX, true);
                }
            }
            // Landing
            else if (!wasGrounded && isGrounded)
            {
                state = EntityState.Landing;
                if (HasProperty(EntityProperty.FallingOnSpawn))
                    StartFalling(false);
                
                {
                    playerVFX.audio = AudioType.Player_Land;
                    playerVFX.scaleOffset = new Vector2(.25f, -.25f);
                    velocity.x = 0; // NOTE: This's for resetting the delta velocity for start/stop moving
                    
                    CapsuleCollider2D capsule = cd as CapsuleCollider2D;
                    if (capsule)
                    {
                        capsule.direction = CapsuleDirection2D.Horizontal;
                        playerVFX.done = () => capsule.direction = CapsuleDirection2D.Vertical;
                    }
                }
            }
            // Start falling
            else if (wasGrounded && !isGrounded)
            {
                state = EntityState.Falling;
                // TODO: Has a falling animation rather than the first frame of the idle one
                playerVFX = new EntityVFX { properties = new Property<VFXProperty>(VFXProperty.StopAnimation), nextAnimation = "Idle" };
            }
            else
            {
                playerVFX = null;
                if (isGrounded)
                {
                    Vector2 deltaVelocity = velocity - prevVelocity;
                    if (velocity.x != 0 && Time.time > timeBtwFootstepsValue)
                    {
                        timeBtwFootstepsValue = Time.time + timeBtwFootsteps.randomValue;
                        playerVFX = new EntityVFX() { audio = footstepAudio };
                    }
                    
                    if (deltaVelocity.x != 0)
                    {
                        if (playerVFX == null)
                            playerVFX = new EntityVFX();
                        playerVFX.particles = new ParticleSystem[] { deltaVelocity.x > 0 ? leftDust : rightDust };
                        if (prevVelocity.x == 0)
                        {
                            state = EntityState.StartMoving;
                            playerVFX.nextAnimation = "Move";
                        }
                        else if (velocity.x == 0)
                        {
                            state = EntityState.StopMoving;
                            playerVFX.nextAnimation = "Idle";
                        }
                    }
                }
            }
            
#if !SCRIPTABLE_VFX
            // NOTE: Currently, only the player has a jumping/landing/falling VFX, but that will probably change soon.
            // When that happens, remember to abstract this code out. Currently, we have a check that only the player can call PlayVFX here.
            if (GameManager.player == this)
                PlayVFX(playerVFX);
#else
            if (vfx)
                foreach (var effect in vfx.items[state])
                    StartCoroutine(PlayVFX(effect));
#endif
            state = EntityState.None;
        }
    }
    
    public IEnumerator PlayVFX(VFX vfx)
    {
        if (vfx == null)
            yield break;
        Debug.Log(vfx.name);
        
        if (vfx.timeline.min > 0)
            yield return new WaitForSeconds(vfx.timeline.min);
        System.Action after = () => { };
        IEnumerator[] enumerators = new IEnumerator[4];
        int enumeratorCount = 0;
        
        // Position/Scale
        {
            float duration = vfx.flags.HasProperty(VFXFlag.OverTime) ? vfx.timeline.range : 0;
            if (vfx.flags.HasProperty(VFXFlag.OffsetPosition))
                Offset(() => transform.position, p => transform.position = p);
            if (vfx.flags.HasProperty(VFXFlag.OffsetScale) && vfx.type != VFXType.Trail)
                Offset(() => transform.localScale, s => transform.localScale = s);
            
            void Offset(System.Func<Vector3> getter, System.Action<Vector3> setter)
            {
                StartCoroutine(ChangeOverTime(p => setter(p), getter(), getter() + (Vector3)vfx.offset, duration));
                after += () => StartCoroutine(ChangeOverTime(p => setter(p), getter(), getter() - (Vector3)vfx.offset, vfx.stayTime));
                //enumerators[enumeratorCount++] = ChangeOverTime(p => setter(p), getter(), getter() - (Vector3)vfx.offset, vfx.stayTime);
                
                static IEnumerator ChangeOverTime(System.Action<Vector3> setValue, Vector3 startValue, Vector3 endValue, float decreaseTime)
                {
                    if (decreaseTime > 0)
                    {
                        float t = 0;
                        while (t <= 1)
                        {
                            setValue(Vector3.Lerp(startValue, endValue, t));
                            t += Time.deltaTime / decreaseTime;
                            yield return null;
                        }
                    }
                    setValue(endValue);
                    //Debug.Log($"{vfx.name}: {Time.frameCount}, {startValue}, {endValue}");
                }
            }
        }
        
        // Rotation
        after += () =>
        {
            Vector3 rotation = Vector3.zero;
            if (vfx.flags.HasProperty(VFXFlag.FlipX))
                rotation.x = 180;
            if (vfx.flags.HasProperty(VFXFlag.FlipY))
                rotation.y = 180;
            if (vfx.flags.HasProperty(VFXFlag.FlipZ))
                rotation.z = 180;
            transform.Rotate(rotation);
        };
        
        // Animation
        if (anim)
        {
            if (!string.IsNullOrEmpty(vfx.animation))
                anim.Play(vfx.animation);
            if (vfx.flags.HasProperty(VFXFlag.StopAnimation))
                anim.speed = 0;
            if (vfx.flags.HasProperty(VFXFlag.ResumeAnimation))
                after += () => anim.speed = 1; // NOTE: Maybe resume animation instantly
        }
        
        // Other
        {
            if (vfx.flags.HasProperty(VFXFlag.StopTime))
                StartCoroutine(GameUtils.StopTime(vfx.timeline.range));
            if (vfx.flags.HasProperty(VFXFlag.ToggleCurrent))
                gameObject.SetActive(!gameObject.activeSelf);
            
            ParticleEffect.instance.SpawnParticle(vfx.particleType, transform.position, vfx.size);
            AudioManager.PlayAudio(vfx.audio);
            CameraSystem.instance.Shake(vfx.shakeMode);
            CameraSystem.instance.Shock(vfx.speed, vfx.size);
            
            if (vfx.pools != null)
                foreach (PoolType pool in vfx.pools)
                    ObjectPooler.Spawn(pool, transform.position);
        }
        
        switch (vfx.type)
        {
            case VFXType.Camera:
            {
                StartCoroutine(CameraSystem.instance.Flash(vfx.timeline.range, vfx.color.a));
            }
            break;
            case VFXType.Flash:
            {
                StartCoroutine(Flashing(sr, whiteMat, vfx.color, vfx.timeline.range, vfx.stayTime));
                
                static IEnumerator Flashing(SpriteRenderer sr, Material whiteMat, Color color, float duration, float flashTime)
                {
                    if (whiteMat == null)
                        yield break;
                    
                    whiteMat.color = color;
                    Debug.Log("Start flashing");
                    
                    while (duration > 0)
                    {
                        float currentTime = Time.time;
                        Material defMat = sr.material;
                        sr.material = whiteMat;
                        yield return new WaitForSeconds(flashTime);
                        
                        sr.material = defMat;
                        yield return new WaitForSeconds(flashTime);
                        duration -= Time.time - currentTime;
                    }
                    
                    whiteMat.color = Color.white;
                    Debug.Log("End flashing");
                }
            }
            break;
            case VFXType.Fade:
            {
                StartCoroutine(DecreaseOverTime(alpha => sr.color += new Color(0, 0, 0, alpha - sr.color.a), vfx.color.a, vfx.timeline.range));
            }
            break;
            case VFXType.Trail:
            {
                StartCoroutine(EnableTrail(trail, vfx.timeline.range, vfx.stayTime));
                if (vfx.flags.HasProperty(VFXFlag.OffsetScale))
                    after += () => StartCoroutine(DecreaseOverTime(width => trail.widthMultiplier = width, trail.widthMultiplier, vfx.stayTime));
                //enumerators[enumeratorCount++] = DecreaseOverTime(width => trail.widthMultiplier = width, trail.widthMultiplier, vfx.stayTime);
                if (vfx.flags.HasProperty(VFXFlag.FadeOut))
                {
                    // TODO: Fade trail
                }
                
                static IEnumerator EnableTrail(TrailRenderer trail, float emitTime, float stayTime)
                {
                    trail.enabled = true;
                    trail.emitting = true;
                    yield return new WaitForSeconds(emitTime);
                    
                    trail.emitting = false;
                    yield return new WaitForSeconds(stayTime);
                    
                    trail.Clear();
                    trail.emitting = true;
                    trail.enabled = false;
                }
            }
            break;
            case VFXType.Text:
            {
                TMPro.TextMeshPro text = GetComponent<TMPro.TextMeshPro>();
                if (vfx.color != Color.clear)
                    text.color = vfx.color;
                if (vfx.size != 0)
                    text.fontSize = vfx.size;
                if (vfx.flags.HasProperty(VFXFlag.FadeOut))
                    after += () => StartCoroutine(DecreaseOverTime(alpha => text.alpha = alpha, text.alpha, vfx.stayTime));
                //enumerators[enumeratorCount++] = DecreaseOverTime(alpha => text.alpha = alpha, text.alpha, vfx.stayTime);
            }
            break;
        }
        
        yield return new WaitForSeconds(vfx.timeline.range);
        after();
        /*Coroutine[] coroutines = new Coroutine[enumeratorCount];
        for (int i = 0; i < enumeratorCount; i++)
        coroutines[i] = StartCoroutine(enumerators[i]);
        foreach (Coroutine coroutine in coroutines)
        {
        yield return coroutine;
        }*/
        
        // TODO: Maybe change this to an offset-based
        static IEnumerator DecreaseOverTime(System.Action<float> setValue, float startValue, float decreaseTime)
        {
            if (decreaseTime > 0)
            {
                float t = 0;
                while (t <= 1)
                {
                    setValue(Mathf.Lerp(startValue, 0, t));
                    t += Time.deltaTime / decreaseTime;
                    yield return null;
                }
                yield return new WaitForEndOfFrame();
                setValue(startValue);
            }
        }
    }
    
    public enum TargetType
    {
        None,
        Input,
        Player,
        Random,
        MoveDir,
        MoveRegion,
        Target,
        
        Count
    }
    
    public enum MoveType
    {
        None,
        Run,
        Fly,
        SmoothDamp,
        Custom,
        Spring,
        
        Count
    }
    
    void MoveEntity()
    {
        SetProperty(EntityProperty.AtEndOfMoveRegion, false);
        switch (targetType)
        {
            case TargetType.Input:
            {
                targetDir = new Vector2(GameInput.GetAxis(AxisType.Horizontal), GameInput.GetAxis(AxisType.Vertical));
            } break;
            case TargetType.Player:
            {
                targetPos = GameManager.player.transform.position;
            } goto case TargetType.Target;
            case TargetType.Random:
            {
                targetDir = MathUtils.RandomVector2();
            } break;
            case TargetType.MoveDir:
            {
                targetDir = transform.right;
            } break;
            case TargetType.MoveRegion:
            {
                if (targetPos != moveRegion.max && targetPos != moveRegion.min)
                    SwitchTargetToMax(true, false);
                else if (targetPos == moveRegion.max && MathUtils.IsApproximate(transform.position, moveRegion.max, .001f, +1))
                    SwitchTargetToMax(false, true);
                else if (targetPos == moveRegion.min && MathUtils.IsApproximate(transform.position, moveRegion.min, .001f, -1))
                    SwitchTargetToMax(true, true);
                
                void SwitchTargetToMax(bool toMax, bool atEndOfMoveRegion)
                {
                    targetPos = toMax ? moveRegion.max : moveRegion.min;
                    velocity = Vector2.zero;
                    SetProperty(EntityProperty.AtEndOfMoveRegion, atEndOfMoveRegion);
                    if (!atEndOfMoveRegion && HasProperty(EntityProperty.StartAtMinMoveRegion))
                        transform.position = moveRegion.min.Z(transform.position.z);
                }
            } goto case TargetType.Target;
            case TargetType.Target:
            {
                switch (offsetType)
                {
                    case TargetOffsetType.Mouse:  { offsetDir = GameInput.GetMouseDir();                  } break;
                    case TargetOffsetType.Player: { offsetDir = GameManager.player.transform.Direction(); } break;
                }
                targetPos += new Vector2(offsetDir * targetOffset);
                targetDir = targetPos - (Vector2)transform.position;
            } break;
        }
        
        switch (moveType)
        {
            case MoveType.None:
            {
                velocity = Vector2.zero;
            } break;
            case MoveType.Run:
            {
                targetDir.x = MathUtils.Sign(targetDir.x);
                targetDir.y = 0;
                velocity = new Vector2(targetDir.x * speed, rb.velocity.y);
            } break;
            case MoveType.Fly:
            {
                velocity = targetDir.normalized * speed;
            } break;
            case MoveType.SmoothDamp: // TODO: Do we still need SmoothDamp? MoveType.Spring seems to also include this already.
            {
                transform.position = MathUtils.SmoothDamp(transform.position, targetPos, ref velocity, new Vector2(speed, speedY),
                                                          Time.deltaTime, transform.position.z);
            } return;
            case MoveType.Spring:
            {
                Vector2 newPos = targetPos;
                newPos.y = MathUtils.SecondOrder(Time.deltaTime, targetPos.y, transform.position.y, spring);
                //transform.SetPositionAndRotation(newPos, GameManager.player.transform.rotation);
                transform.position = newPos;
            } return;
            case MoveType.Custom: return;
        }
        
        if (rb)
            rb.velocity = velocity;
        else
            transform.position += (Vector3)velocity * Time.deltaTime;
    }
    
    public enum RotateType
    {
        None,
        PlayerX,
        MoveDirX,
        Weapon,
        MouseX,
        Linear,
        
        Count
    }
    
    static void RotateEntity(Transform transform, RotateType rotateType, float dRotate, float velocityX)
    {
        float dirX = 0;
        Transform player = GameManager.player.transform;
        Vector2 mouseDir = GameInput.GetDirToMouse(transform.position);
        switch (rotateType)
        {
            case RotateType.PlayerX:
            {
                dirX = player.position.x - transform.position.x;
            } break;
            case RotateType.MoveDirX:
            {
                dirX = velocityX;
            } break;
            case RotateType.MouseX:
            {
                dirX = mouseDir.x;
            } break;
            case RotateType.Weapon:
            {
                transform.rotation = GameManager.player.fallRemember > 0 ? player.rotation : MathUtils.GetQuaternionFlipY(mouseDir, player.up.y);
            } break;
            case RotateType.Linear:
            {
                transform.Rotate(0, 0, dRotate * Time.deltaTime);
            } break;
        }
        
        if (dirX != 0 && Mathf.Sign(dirX) != Mathf.Sign(transform.right.x))
            transform.Rotate(0, 180, 0);
    }
    
    void StartFalling(bool startFalling)
    {
        rb.velocity = startFalling ? new Vector2(fallDir.randomValue, Random.value).normalized * speed : Vector2.zero;
        moveType = startFalling ? MoveType.Custom : MoveType.Fly;
        cd.isTrigger = !startFalling;
        rb.bodyType = startFalling ? RigidbodyType2D.Dynamic : RigidbodyType2D.Kinematic;
        SetProperty(EntityProperty.FallingOnSpawn, startFalling);
        if (!startFalling)
            CalculateMoveRegion();
    }
    
    public bool CompleteCycle()
    {
        return HasProperty(EntityProperty.AtEndOfMoveRegion) && targetPos == moveRegion.max;
    }
    
    public void TestPlayerVFX()
    {
#if !SCRIPTABLE_VFX
        Hurt(0);
#else
        VFX vfx1 = CreateTestVFX("vfx1", new RangedFloat(.0f, .5f), new Vector2(-.25f, .25f));
        VFX vfx2 = CreateTestVFX("vfx2", new RangedFloat(.3f, .8f), new Vector2(.25f, -.25f));
        
        StartCoroutine(PlayVFX(vfx1));
        StartCoroutine(PlayVFX(vfx2));
        
        static VFX CreateTestVFX(string name, RangedFloat timeline, Vector2 offset)
        {
            VFX vfx = ScriptableObject.CreateInstance<VFX>();
            vfx.name = name;
            vfx.offset = offset;
            vfx.flags.SetProperty(VFXFlag.OffsetScale, true);
            vfx.timeline = timeline;
            return vfx;
        }
#endif
    }
    
    public void Hurt(int damage)
    {
        if (HasProperty(EntityProperty.CanBeHurt))
        {
            health -= damage;
            
            // TODO: Replace this with a stat
            moveType = MoveType.None;
            rotateType = RotateType.None;
            SetProperty(EntityProperty.CanBeHurt, false);
            if (health > 0)
            {
                state = EntityState.OnHit;
                //PlayVFX(hurtVFX);
            }
            else
                Die();
        }
    }
    
    void Die()
    {
        state = EntityState.OnDeath;
        PlayVFX(deathVFX);
    }
    
    public bool HasProperty(EntityProperty property)
    {
        return properties.HasProperty(property);
    }
    
    bool SetProperty(EntityProperty property, bool set)
    {
        properties.SetProperty(property, set);
        return set;
    }
    
    private void OnTriggerEnter2D(Collider2D collision)
    {
        OnHitEnter(collision);
    }
    
    private void OnCollisionEnter2D(Collision2D collision)
    {
        OnHitEnter(collision.collider);
    }
    
    void OnHitEnter(Collider2D collision)
    {
        foreach (string tag in collisionTags)
        {
            if (collision.CompareTag(tag))
            {
                Entity entity;
                if (entity = collision.GetComponent<Entity>())
                {
                    if (HasProperty(EntityProperty.DamageWhenCollide))
                        entity.Hurt(damage);
                    if (HasProperty(EntityProperty.AddMoneyWhenCollide))
                        entity.money += money;
                }
                if (HasProperty(EntityProperty.DieWhenCollide))
                    Die();
                if (HasProperty(EntityProperty.SpawnDamagePopup))
                    ObjectPooler.Spawn<Entity>(PoolType.DamagePopup, transform.position)
                        .InitDamagePopup(damage, HasProperty(EntityProperty.IsCritical));
            }
        }
    }
    
    bool IsInRange(float range, Vector2 targetPos)
    {
        return (targetPos - (Vector2)transform.position).sqrMagnitude < range * range;
    }
    
    public bool IsInRange(Vector2 range, Vector2 targetPos)
    {
        Vector2 targetDir = MathUtils.Abs(targetPos - (Vector2)transform.position);
        return targetDir.x < range.x && targetDir.y < range.y;
    }
    
    public bool IsInRangeY(float range, Vector2 targetPos)
    {
        return Mathf.Abs(targetPos.y - transform.position.y) < range;
    }
    
    void CalculateMoveRegion()
    {
        switch (regionType)
        {
            case MoveRegionType.Ground:
            {
                moveRegion = GameManager.CalculateMoveRegion(transform.position, spriteExtents, -transform.up.y);
            } break;
            case MoveRegionType.Vertical:
            {
                Debug.Assert(transform.up.y == 1);
                if (GameManager.GetGroundPos(transform.position, spriteExtents, -1f, out _, out Vector3Int groundPos))
                {
                    moveRegion = new Rect(new Vector2(transform.position.x, groundPos.y + spriteExtents.y), Vector2.up * verticalHeight);
                    GameDebug.DrawBox(new Rect((Vector3)groundPos, spriteExtents), Color.red);
                }
            } break;
        }
    }
    
#region VFX
    public enum VFXProperty
    {
        StopAnimation,
        ChangeEffectObjBack,
        ScaleOverTime,
        FadeTextWhenDone,
        StartTrailing,
        DecreaseTrailWidth,
        PlayParticleInOrder,
        FlipX,
        FlipY,
        FlipZ,
    }
    
    [SomeOtherThing]
    [Multiple] [System.Serializable]
    public class EntityVFX
    {
        public Property<VFXProperty> properties;
        
        public System.Action done;
        public System.Func<bool> canStop;
        public string nextAnimation;
        public GameObject effectObj;
        public ParticleSystem[] particles;
        
        [Header("Time")]
        public float waitTime;
        public float scaleTime;
        public float rotateTime;
        
        [Header("Trail Effect")]
        public float trailEmitTime;
        public float trailStayTime;
        
        [Header("Flashing")]
        public float flashTime;
        public float flashDuration;
        public Color triggerColor;
        
        [Header("Text Effect")]
        public Color textColor;
        public float fontSize;
        
        [Header("Camera Effect")]
        public float stopTime;
        public float trauma;
        public ShakeMode shakeMode;
        public float shockSpeed;
        public float shockSize;
        public float camFlashTime;
        public float camFlashAlpha;
        
        [Header("After Fade")]
        public float alpha;
        public float fadeTime;
        
        [Header("Explode Particle")]
        public float range;
        public ParticleType particleType;
        
        [Header("Other")]
        public AudioType audio;
        public PoolType poolType;
        public Vector2 scaleOffset;
    }
    
    void PlayVFX(EntityVFX vfx)
    {
        if (vfx == null)
            return;
        Debug.Log(state);
        
        if (vfx.properties.HasProperty(VFXProperty.ScaleOverTime))
            StartCoroutine(ScaleOverTime(transform, vfx.scaleTime, vfx.scaleOffset));
        else
            transform.localScale += (Vector3)vfx.scaleOffset;
        
        if (!string.IsNullOrEmpty(vfx.nextAnimation))
            anim.Play(vfx.nextAnimation);
        if (vfx.properties.HasProperty(VFXProperty.StopAnimation))
            anim.speed = 0;
        
        if (vfx.textColor != Color.clear)
            text.color = vfx.textColor;
        if (vfx.fontSize != 0)
            text.fontSize = vfx.fontSize;
        
        float totalParticleTime = 0;
        float particleCount = vfx.particles?.Length ?? 0;
        for (int i = 0; i < particleCount; ++i)
        {
            if (vfx.particles[i])
            {
                this.InvokeAfter(totalParticleTime, () => vfx.particles[i].Play(), true);
                if (vfx.properties.HasProperty(VFXProperty.PlayParticleInOrder))
                    totalParticleTime += vfx.particles[i].main.duration;
            }
        }
        
        if (vfx.effectObj)
            vfx.effectObj.SetActive(!vfx.effectObj.activeSelf);
        
        StartCoroutine(CameraSystem.instance.Flash(vfx.camFlashTime, vfx.camFlashAlpha));
        CameraSystem.instance.Shake(vfx.shakeMode, null);//, vfx.trauma == 0 ? 1 : vfx.trauma);
        CameraSystem.instance.Shock(vfx.shockSpeed, vfx.shockSize);
        
        AudioManager.PlayAudio(vfx.audio);
        ObjectPooler.Spawn(vfx.poolType, transform.position);
        ParticleEffect.instance.SpawnParticle(vfx.particleType, transform.position, vfx.range);
        
        StartCoroutine(GameUtils.StopTime(vfx.stopTime));
        StartCoroutine(Flashing(sr, whiteMat, vfx.triggerColor, vfx.flashDuration, vfx.flashTime, vfx.canStop));
        float x = vfx.properties.HasProperty(VFXProperty.FlipX) ? 180 : 0;
        float y = vfx.properties.HasProperty(VFXProperty.FlipY) ? 180 : 0;
        float z = vfx.properties.HasProperty(VFXProperty.FlipZ) ? 180 : 0;
        if (x != 0 || y != 0 || z != 0)
            this.InvokeAfter(vfx.rotateTime, () => transform.Rotate(new Vector3(x, y, z)));
        
        this.InvokeAfter(Mathf.Max(vfx.flashDuration, totalParticleTime, vfx.scaleTime), () =>
                         {
                             if (vfx.properties.HasProperty(VFXProperty.FadeTextWhenDone))
                                 StartCoroutine(FadeText(text, vfx.alpha, vfx.fadeTime));
                             else
                                 StartCoroutine(Flashing(sr, whiteMat, new Color(1, 1, 1, vfx.alpha), vfx.fadeTime, vfx.fadeTime, vfx.canStop));
                             
                             if (vfx.properties.HasProperty(VFXProperty.StartTrailing))
                                 StartCoroutine(EnableTrail(trail, vfx.trailEmitTime, vfx.trailStayTime));
                             if (vfx.properties.HasProperty(VFXProperty.DecreaseTrailWidth))
                                 this.InvokeAfter(vfx.trailEmitTime, () => StartCoroutine(DecreaseTrailWidth(trail, vfx.trailStayTime)));
                             
                             this.InvokeAfter(Mathf.Max(vfx.fadeTime, vfx.trailEmitTime + vfx.trailStayTime) + vfx.waitTime, () =>
                                              {
                                                  if (!vfx.properties.HasProperty(VFXProperty.ScaleOverTime))
                                                      transform.localScale -= (Vector3)vfx.scaleOffset;
                                                  if (vfx.properties.HasProperty(VFXProperty.ChangeEffectObjBack))
                                                      vfx.effectObj.SetActive(!vfx.effectObj.activeSelf);
                                                  if (anim)
                                                      anim.speed = 1;
                                                  vfx.done?.Invoke();
                                              });
                         });
        
        static IEnumerator Flashing(SpriteRenderer sr, Material whiteMat, Color color, float duration, float flashTime, System.Func<bool> canStop)
        {
            if (whiteMat == null)
                yield break;
            
            whiteMat.color = color;
            
            while (duration > 0)
            {
                if (canStop?.Invoke() ?? false)
                    break;
                
                float currentTime = Time.time;
                Material defMat = sr.material;
                sr.material = whiteMat;
                yield return new WaitForSeconds(flashTime);
                
                sr.material = defMat;
                yield return new WaitForSeconds(flashTime);
                duration -= Time.time - currentTime;
            }
            
            whiteMat.color = Color.white;
        }
        
        static IEnumerator ScaleOverTime(Transform transform, float duration, Vector3 scaleOffset)
        {
            while (duration > 0)
            {
                duration -= Time.deltaTime;
                transform.localScale += scaleOffset * Time.deltaTime;
                yield return null;
            }
            
            while (transform.gameObject.activeSelf)
            {
                transform.localScale -= scaleOffset * Time.deltaTime;
                yield return null;
            }
        }
        
        static IEnumerator FadeText(TMPro.TextMeshPro text, float alpha, float fadeTime)
        {
            float dAlpha = (text.alpha - alpha) / fadeTime;
            while (text.alpha > alpha)
            {
                text.alpha -= dAlpha * Time.deltaTime;
                yield return null;
            }
        }
        
        static IEnumerator EnableTrail(TrailRenderer trail, float emitTime, float stayTime)
        {
            trail.enabled = true;
            trail.emitting = true;
            yield return new WaitForSeconds(emitTime);
            
            trail.emitting = false;
            yield return new WaitForSeconds(stayTime);
            
            trail.Clear();
            trail.emitting = true;
            trail.enabled = false;
        }
        
        static IEnumerator DecreaseTrailWidth(TrailRenderer trail, float decreaseTime)
        {
            float startWidth = trail.widthMultiplier;
            float startTime = decreaseTime;
            while (decreaseTime > 0)
            {
                trail.widthMultiplier = decreaseTime / startTime * startWidth;
                decreaseTime -= Time.deltaTime;
                yield return null;
            }
            trail.widthMultiplier = startWidth;
        }
    }
#endregion
}
