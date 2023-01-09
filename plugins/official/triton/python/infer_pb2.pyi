from google.protobuf import empty_pb2 as _empty_pb2
from google.protobuf.internal import containers as _containers
from google.protobuf.internal import enum_type_wrapper as _enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from typing import ClassVar as _ClassVar, Iterable as _Iterable, Mapping as _Mapping, Optional as _Optional, Union as _Union

BYTE: DataType
BYTE2: DataType
BYTE4: DataType
BYTE8: DataType
DESCRIPTOR: _descriptor.FileDescriptor
UNKNOWN: DataType

class Batch(_message.Message):
    __slots__ = ["tensor"]
    TENSOR_FIELD_NUMBER: _ClassVar[int]
    tensor: _containers.RepeatedCompositeFieldContainer[Tensor]
    def __init__(self, tensor: _Optional[_Iterable[_Union[Tensor, _Mapping]]] = ...) -> None: ...

class InferRequest(_message.Message):
    __slots__ = ["inputs", "name", "request_id"]
    INPUTS_FIELD_NUMBER: _ClassVar[int]
    NAME_FIELD_NUMBER: _ClassVar[int]
    REQUEST_ID_FIELD_NUMBER: _ClassVar[int]
    inputs: _containers.RepeatedCompositeFieldContainer[Batch]
    name: str
    request_id: int
    def __init__(self, name: _Optional[str] = ..., request_id: _Optional[int] = ..., inputs: _Optional[_Iterable[_Union[Batch, _Mapping]]] = ...) -> None: ...

class InferResponse(_message.Message):
    __slots__ = ["error_code", "msg", "name", "outputs", "request_id"]
    ERROR_CODE_FIELD_NUMBER: _ClassVar[int]
    MSG_FIELD_NUMBER: _ClassVar[int]
    NAME_FIELD_NUMBER: _ClassVar[int]
    OUTPUTS_FIELD_NUMBER: _ClassVar[int]
    REQUEST_ID_FIELD_NUMBER: _ClassVar[int]
    error_code: int
    msg: str
    name: str
    outputs: _containers.RepeatedCompositeFieldContainer[Batch]
    request_id: int
    def __init__(self, name: _Optional[str] = ..., request_id: _Optional[int] = ..., error_code: _Optional[int] = ..., msg: _Optional[str] = ..., outputs: _Optional[_Iterable[_Union[Batch, _Mapping]]] = ...) -> None: ...

class LoadRequest(_message.Message):
    __slots__ = ["name"]
    NAME_FIELD_NUMBER: _ClassVar[int]
    name: str
    def __init__(self, name: _Optional[str] = ...) -> None: ...

class ModelRepos(_message.Message):
    __slots__ = ["model"]
    MODEL_FIELD_NUMBER: _ClassVar[int]
    model: _containers.RepeatedCompositeFieldContainer[ModelStatus]
    def __init__(self, model: _Optional[_Iterable[_Union[ModelStatus, _Mapping]]] = ...) -> None: ...

class ModelStatus(_message.Message):
    __slots__ = ["batch_size", "inputs", "name", "outputs", "running"]
    BATCH_SIZE_FIELD_NUMBER: _ClassVar[int]
    INPUTS_FIELD_NUMBER: _ClassVar[int]
    NAME_FIELD_NUMBER: _ClassVar[int]
    OUTPUTS_FIELD_NUMBER: _ClassVar[int]
    RUNNING_FIELD_NUMBER: _ClassVar[int]
    batch_size: int
    inputs: _containers.RepeatedCompositeFieldContainer[Tensor]
    name: str
    outputs: _containers.RepeatedCompositeFieldContainer[Tensor]
    running: int
    def __init__(self, name: _Optional[str] = ..., batch_size: _Optional[int] = ..., inputs: _Optional[_Iterable[_Union[Tensor, _Mapping]]] = ..., outputs: _Optional[_Iterable[_Union[Tensor, _Mapping]]] = ..., running: _Optional[int] = ...) -> None: ...

class Tensor(_message.Message):
    __slots__ = ["data", "dtype", "name", "shape"]
    DATA_FIELD_NUMBER: _ClassVar[int]
    DTYPE_FIELD_NUMBER: _ClassVar[int]
    NAME_FIELD_NUMBER: _ClassVar[int]
    SHAPE_FIELD_NUMBER: _ClassVar[int]
    data: bytes
    dtype: DataType
    name: str
    shape: _containers.RepeatedScalarFieldContainer[int]
    def __init__(self, name: _Optional[str] = ..., shape: _Optional[_Iterable[int]] = ..., dtype: _Optional[_Union[DataType, str]] = ..., data: _Optional[bytes] = ...) -> None: ...

class DataType(int, metaclass=_enum_type_wrapper.EnumTypeWrapper):
    __slots__ = []
